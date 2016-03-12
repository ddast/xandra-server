/*  xandra - receive and print unicode characters
 *
 *  Copyright (C) 2016 Dennis Dast <mail@ddast.de>
 *
 *  This file is part of xandra.
 *
 *  xandra is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  xandra is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with xandra.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <linux/tcp.h>

#include "server.h"
#include "unicode.h"

#ifdef DEBUG
# define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
# define DEBUG_PRINT(...) do {} while (0)
#endif

#define TIMEOUT 9

#define RECVBUFSIZE 1024
#define KEYSEQLEN   11

#define MOUSEEVENT    127
#define MOUSEEVENTLEN 9

#define RESERVEDCHARS 32
#define HEARTBEAT     0x00
#define LEFTCLICK     0x01
#define RIGHTCLICK    0x02
#define BACKSPACE     0x03
#define ESCAPE        0x04
#define TAB           0x05
#define LEFT          0x06
#define DOWN          0x07
#define UP            0x08
#define RIGHT         0x09
#define RETURN        0x0a
#define CTRL          0x0b
#define SUPER         0x0c
#define ALT           0x0d

#define SPECIALKEYSOFFSET 3
#define SPECIALKEYSLEN    11
static const char * const special_keys[SPECIALKEYSLEN] = {
  "BackSpace",
  "Escape",
  "Tab",
  "Left",
  "Down",
  "Up",
  "Right",
  "Return",
  "Ctrl",
  "Super",
  "Alt"
};
static char modifier_and_key[2*KEYSEQLEN+1];

// Returns the presentation IP4 or IP6 address stored in a sockaddr_storage.
void* get_in_addr(struct sockaddr* addr);

// Receives and forwards data until connection is closed by peer or times out.
void receive(int sfd, xdo_t* xdo);

// Processes nbytes in buffer.  Possible actions are
// 0:          heartbeat
// 1:          send backspace
// utf8 char:  send character using xdo
// else:       do nothing
void process_input(const unsigned char* buffer, int nbytes, xdo_t* xdo);

// Print buffer for debugging.
void print_buffer(const unsigned char* buffer, int nbytes);


void print_welcome()
{
  char hostname[128];
  if (gethostname(hostname, sizeof hostname) == 0) {
    printf("Starting server on '%s'...\n", hostname);
  } else {
    perror("gethostname");
    exit(EXIT_FAILURE);
  }
}


void* get_in_addr(struct sockaddr* addr)
{
  if (addr->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)addr)->sin_addr);
  }
  return &(((struct sockaddr_in6*)addr)->sin6_addr);
}


int get_socket(char* port)
{
  struct addrinfo hints, *result;
  memset(&hints, 0, sizeof hints);
  hints.ai_family   = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags    = AI_PASSIVE;

  int status = getaddrinfo(NULL, port, &hints, &result);
  if (status != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    exit(EXIT_FAILURE);
  }

  struct addrinfo* rp;
  int sfd;
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sfd == -1) {
      perror("socket");
      continue;
    }

    int yes = 1;
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      perror("setsockopt");
      exit(EXIT_FAILURE);
    }

    struct timeval tv = { .tv_sec = TIMEOUT, .tv_usec = 0 };
    if (setsockopt(sfd, SOL_SOCKET, SO_RCVTIMEO,
                   (char*)&tv, sizeof(struct timeval)) == -1) {
      perror("setsockopt");
      exit(EXIT_FAILURE);
    }

    if (setsockopt(sfd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(int)) == -1) {
      perror("setsockopt");
      exit(EXIT_FAILURE);
    }

    if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == -1) {
      close(sfd);
      perror("bind");
      continue;
    }

    break;
  }

  freeaddrinfo(result);

  if (rp == NULL) {
    fprintf(stderr, "Could not bind\n");
    exit(EXIT_FAILURE);
  }

  if (listen(sfd, 0) == -1) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  return sfd;
}


void wait_and_receive(int sfd, xdo_t* xdo)
{
  struct sockaddr_storage peer_addr;
  socklen_t peer_addr_len = sizeof peer_addr;
  while (1) {
    int peer_sfd = accept(sfd, (struct sockaddr*)&peer_addr, &peer_addr_len);
    if (peer_sfd == -1) {
      // do not spam timeout messages on accept
      // perror("accept");
      continue;
    }

    char inet_addr[INET6_ADDRSTRLEN];
    inet_ntop(peer_addr.ss_family, get_in_addr((struct sockaddr*)&peer_addr),
              inet_addr, sizeof inet_addr);
    printf("Connected to %s\n", inet_addr);

    receive(peer_sfd, xdo);
    printf("Connection closed\n");
    close(peer_sfd);
  }
}


void receive(int sfd, xdo_t* xdo)
{
  unsigned char buffer[RECVBUFSIZE];
  int nbytes = -1;
  // do not fill the last three chars of the buffer to prevent a buffer
  // overflow if the last char looks like a four byte utf8 code
  while ((nbytes = recv(sfd, buffer, RECVBUFSIZE-3, 0)) != 0) {
    if (nbytes == -1) {
      perror("recv");
      return;
    }
    process_input(buffer, nbytes, xdo);
  }
}


void process_input(const unsigned char* buffer, int nbytes, xdo_t* xdo)
{
  if (buffer[0] == MOUSEEVENT) {
    // mouse event
    if (nbytes < MOUSEEVENTLEN) {
      fprintf(stderr, "Received malformatted mouse event\n");
      print_buffer(buffer, nbytes);
      return;
    }
    int32_t distanceX = buffer[1]<<24 | buffer[2]<<16 |
                        buffer[3]<<8 | buffer[4];
    int32_t distanceY = buffer[5]<<24 | buffer[6]<<16 |
                        buffer[7]<<8 | buffer[8];
    xdo_move_mouse_relative(xdo, distanceX, distanceY);
    if (nbytes > MOUSEEVENTLEN) {
      process_input(buffer + MOUSEEVENTLEN, nbytes - MOUSEEVENTLEN, xdo);
    }
  } else {
    // keyboard event
    int processed_bytes = 0;
    while (processed_bytes < nbytes) {
      int32_t unicode;
      processed_bytes += utf8_to_unicode(buffer+processed_bytes, &unicode);
      if (unicode == 0) {
        DEBUG_PRINT("Received heartbeat\n");
        continue;
      }
      if (unicode >= RESERVEDCHARS) {
        char keysequence[KEYSEQLEN];
        snprintf(keysequence, KEYSEQLEN, "%#010x", unicode);
        strcat(modifier_and_key, keysequence);
        DEBUG_PRINT("Sending '%s'\n", modifier_and_key);
        xdo_send_keysequence_window(xdo, CURRENTWINDOW, modifier_and_key,
                                    12000);
      } else if (unicode < 0) {
        fprintf(stderr, "Received unknown character\n");
        print_buffer(buffer, nbytes);
      } else if (unicode == LEFTCLICK) {
        DEBUG_PRINT("Sending 'Leftclick'\n");
        xdo_click_window(xdo, CURRENTWINDOW, 1);
      } else if (unicode == RIGHTCLICK) {
        DEBUG_PRINT("Sending 'Rightclick'\n");
        xdo_click_window(xdo, CURRENTWINDOW, 3);
      } else if (unicode == CTRL || unicode == SUPER || unicode == ALT) {
        strcpy(modifier_and_key, special_keys[unicode-SPECIALKEYSOFFSET]);
        strcat(modifier_and_key, "+");
        continue;
      } else if (unicode-SPECIALKEYSOFFSET < SPECIALKEYSLEN) {
        strcat(modifier_and_key, special_keys[unicode-SPECIALKEYSOFFSET]);
        DEBUG_PRINT("Sending '%s'\n", modifier_and_key);
        xdo_send_keysequence_window(xdo, CURRENTWINDOW, modifier_and_key,
                                    12000);
      }
      modifier_and_key[0] = '\0';
    }
  }
}

void print_buffer(const unsigned char* buffer, int nbytes)
{
  fprintf(stderr, "nbytes: %d\n", nbytes);
  for (int i = 0; i < nbytes; ++i) {
    fprintf(stderr, "0x%x ", buffer[i]);
  }
  fprintf(stderr, "\n");
}
