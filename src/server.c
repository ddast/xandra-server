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
#define KEYSEQLEN   20

#define MOUSEEVENT    0xff
#define MOUSEEVENTLEN 9

#define HEARTBEAT 0x00

#define SPECIALKEY 0xfe

#define MOUSECLICKSOFFSET 0x00
#define MOUSECLICKSLEN    4
static const int mouse_clicks[MOUSECLICKSLEN] = {
  1,  // LEFTCLICK  0x00
  3,  // RIGHTCLICK 0x01
  4,  // WHEELUP    0x02
  5   // WHEELDOWN  0x03
};

#define MODIFIERKEYSOFFSET 0x04
#define MODIFIERKEYSLEN    3
static const char * const modifier_keys[MODIFIERKEYSLEN] = {
  "Ctrl",  // 0x04
  "Super", // 0x05
  "Alt"    // 0x06
};

#define SPECIALKEYSOFFSET 0x07
#define SPECIALKEYSLEN    29
static const char * const special_keys[SPECIALKEYSLEN] = {
  "BackSpace",             // 0x07
  "Escape",                // 0x08
  "Tab",                   // 0x09
  "Left",                  // 0x0a
  "Down",                  // 0x0b
  "Up",                    // 0x0c
  "Right",                 // 0x0d
  "XF86AudioLowerVolume",  // 0x0e
  "XF86AudioRaiseVolume",  // 0x0f
  "XF86AudioMute",         // 0x10
  "Insert",                // 0x11
  "Delete",                // 0x12
  "Home",                  // 0x13
  "End",                   // 0x14
  "Page_Up",               // 0x15
  "Page_Down",             // 0x16
  "F1",                    // 0x17
  "F2",                    // 0x18
  "F3",                    // 0x19
  "F4",                    // 0x1a
  "F5",                    // 0x1b
  "F6",                    // 0x1c
  "F7",                    // 0x1d
  "F8",                    // 0x1e
  "F9",                    // 0x1f
  "F10",                   // 0x20
  "F11",                   // 0x21
  "F12",                   // 0x22
};

static char modifier_and_key[2*KEYSEQLEN+1];

// Returns the presentation IP4 or IP6 address stored in a sockaddr_storage.
void* get_in_addr(struct sockaddr* addr);

// Receives and forwards data until connection is closed by peer or times out.
void receive(int sfd, xdo_t* xdo);

// Processes nbytes in buffer.
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


void accept_and_receive(int sfd, xdo_t* xdo)
{
  struct sockaddr_storage peer_addr;
  socklen_t peer_addr_len = sizeof peer_addr;
  int peer_sfd = -1;
  while (peer_sfd == -1) {
    peer_sfd = accept(sfd, (struct sockaddr*)&peer_addr, &peer_addr_len);
  }
  close(sfd);

  char inet_addr[INET6_ADDRSTRLEN];
  inet_ntop(peer_addr.ss_family, get_in_addr((struct sockaddr*)&peer_addr),
            inet_addr, sizeof inet_addr);
  printf("Connected to %s\n", inet_addr);

  receive(peer_sfd, xdo);
  close(peer_sfd);
  printf("Connection closed\n");
}


void receive(int sfd, xdo_t* xdo)
{
  unsigned char buffer[RECVBUFSIZE];
  int nbytes = -1;
  // do not fill the last (KEYSEQLEN-1) chars of the buffer to prevent a buffer
  // overflow if the last character misleadlingly looks like the longest key
  // sequence
  while ((nbytes = recv(sfd, buffer, RECVBUFSIZE-KEYSEQLEN+1, 0)) != 0) {
    if (nbytes == -1) {
      perror("recv");
      return;
    }
    process_input(buffer, nbytes, xdo);
  }
}


void process_input(const unsigned char* buffer, int nbytes, xdo_t* xdo)
{
  int processed_bytes = 0, flush_modifier = 0;
  if (buffer[0] == HEARTBEAT) {
    DEBUG_PRINT("Received heartbeat\n");
    processed_bytes = 1;
  } else if (buffer[0] == MOUSEEVENT) {
    if (nbytes < MOUSEEVENTLEN) {
      fprintf(stderr, "Received malformatted mouse event\n");
      print_buffer(buffer, nbytes);
      return;
    }
    int32_t distanceX = buffer[1]<<24 | buffer[2]<<16 |
                        buffer[3]<<8  | buffer[4];
    int32_t distanceY = buffer[5]<<24 | buffer[6]<<16 |
                        buffer[7]<<8  | buffer[8];
    xdo_move_mouse_relative(xdo, distanceX, distanceY);
    processed_bytes = MOUSEEVENTLEN;
  } else if (buffer[0] == SPECIALKEY) {
    if (buffer[1] < MOUSECLICKSOFFSET + MOUSECLICKSLEN) {
      int mouse_button = mouse_clicks[buffer[1]-MOUSECLICKSOFFSET];
      DEBUG_PRINT("Sending mouse click %d\n", mouse_button);
      xdo_click_window(xdo, CURRENTWINDOW, mouse_button);
    } else if (buffer[1] < MODIFIERKEYSOFFSET + MODIFIERKEYSLEN) {
      const char* cur_modifier = modifier_keys[buffer[1]-MODIFIERKEYSOFFSET];
      if (!strncmp(modifier_and_key, cur_modifier, strlen(cur_modifier))) {
        DEBUG_PRINT("Sending '%s'\n", cur_modifier);
        xdo_send_keysequence_window(xdo, CURRENTWINDOW, cur_modifier, 12000);
        flush_modifier = 1;
      } else {
        strcpy(modifier_and_key, cur_modifier);
        strcat(modifier_and_key, "+");
      }
    } else if (buffer[1] < SPECIALKEYSOFFSET + SPECIALKEYSLEN) {
      strcat(modifier_and_key, special_keys[buffer[1]-SPECIALKEYSOFFSET]);
      DEBUG_PRINT("Sending '%s'\n", modifier_and_key);
      xdo_send_keysequence_window(xdo, CURRENTWINDOW, modifier_and_key, 12000);
      flush_modifier = 1;
    }
    processed_bytes = 2;
  } else {
    int32_t unicode;
    processed_bytes = utf8_to_unicode(buffer, &unicode);
    if (unicode >= 0) {
      char keysequence[KEYSEQLEN];
      if (unicode == 0x0a) {
        snprintf(keysequence, KEYSEQLEN, "Return");
      } else {
        snprintf(keysequence, KEYSEQLEN, "%#010x", unicode);
      }
      strcat(modifier_and_key, keysequence);
      DEBUG_PRINT("Sending '%s'\n", modifier_and_key);
      xdo_send_keysequence_window(xdo, CURRENTWINDOW, modifier_and_key, 12000);
      flush_modifier = 1;
    } else {
      fprintf(stderr, "Received unknown character\n");
      print_buffer(buffer, nbytes);
    }
  }
  if (flush_modifier) {
    modifier_and_key[0] = '\0';
  }
  if (nbytes > processed_bytes) {
    process_input(buffer + processed_bytes, nbytes - processed_bytes, xdo);
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
