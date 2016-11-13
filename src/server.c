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
#include <stdint.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "server.h"
#include "unicode.h"

#ifdef DEBUG
# define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
# define DEBUG_PRINT(...) do {} while (0)
#endif

#define TIMEOUT 3

#define RECVBUFSIZE 128
#define KEYSEQLEN   20

#define HEARTBEAT 0x00

#define MOUSECLICKSOFFSET 0x00
#define MOUSECLICKSLEN    5
static const int mouse_clicks[MOUSECLICKSLEN] = {
  1,  // LEFTCLICK   0x00
  2,  // MIDDLECLICK 0x01
  3,  // RIGHTCLICK  0x02
  4,  // WHEELUP     0x03
  5   // WHEELDOWN   0x04
};

#define MODIFIERKEYSOFFSET 0x05
#define MODIFIERKEYSLEN    3
static const char * const modifier_keys[MODIFIERKEYSLEN] = {
  "Ctrl",  // 0x05
  "Super", // 0x06
  "Alt"    // 0x07
};

#define SPECIALKEYSOFFSET 0x08
#define SPECIALKEYSLEN    28
static const char * const special_keys[SPECIALKEYSLEN] = {
  "BackSpace",             // 0x08
  "Escape",                // 0x09
  "Tab",                   // 0x0a
  "Left",                  // 0x0b
  "Down",                  // 0x0c
  "Up",                    // 0x0d
  "Right",                 // 0x0e
  "XF86AudioLowerVolume",  // 0x0f
  "XF86AudioRaiseVolume",  // 0x10
  "XF86AudioMute",         // 0x11
  "Insert",                // 0x12
  "Delete",                // 0x13
  "Home",                  // 0x14
  "End",                   // 0x15
  "Page_Up",               // 0x16
  "Page_Down",             // 0x17
  "F1",                    // 0x18
  "F2",                    // 0x19
  "F3",                    // 0x1a
  "F4",                    // 0x1b
  "F5",                    // 0x1c
  "F6",                    // 0x1d
  "F7",                    // 0x1e
  "F8",                    // 0x1f
  "F9",                    // 0x20
  "F10",                   // 0x21
  "F11",                   // 0x22
  "F12",                   // 0x23
};

// Returns the presentation IP4 or IP6 address stored in a sockaddr_storage.
void* get_in_addr(struct sockaddr* addr);

// Receives and forwards data to process_input until connection is closed by
// peer, times out or running is set to 0.
void receive(int sfd, xdo_t* xdo, int* running);

// Processes maximum nbytes in buffer by repeatedly calling process_character.
// Returns the amount of processed bytes which might by less than nbytes.
size_t process_input(const uint8_t* buffer, size_t nbytes,
                     char* modifier_and_key, xdo_t* xdo);

// Reads one character with at most maxbytes bytes in buffer and processes its
// input.  If the character finishes a keysequence it is executed using xdo.
// If not it is stored in the array modifier_and_key with minimum length
// 2*KEYSEQLEN+1.
// Return value:
//   > 0, the amount of processed bytes
//   -1, malformed input, skip this character
//   -n, buffer contains an n-byte character but n>maxbytes
ssize_t process_character(const uint8_t* buffer, size_t maxbytes,
                          char* modifier_and_key, xdo_t* xdo);

// Print buffer for debugging.
void print_buffer(const uint8_t* buffer, size_t nbytes);


void print_welcome(void)
{
  char hostname[128];
  if (!gethostname(hostname, sizeof hostname)) {
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


int get_socket(const char* port, int protocol)
{
  struct addrinfo hints, *result;
  memset(&hints, 0, sizeof hints);
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags    = AI_PASSIVE;
  switch (protocol) {
    case 4:  hints.ai_family = AF_INET;   break;
    case 6:  hints.ai_family = AF_INET6;  break;
    default: hints.ai_family = AF_UNSPEC; break;
  }

  int status = getaddrinfo(NULL, port, &hints, &result);
  if (status) {
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


void accept_and_receive(int sfd, xdo_t* xdo, int* running)
{
  struct sockaddr_storage peer_addr;
  socklen_t peer_addr_len = sizeof peer_addr;
  int peer_sfd = -1;
  while (peer_sfd == -1) {
    if (!*running){
      close(sfd);
      printf("Shutting down...\n");
      return;
    }
    peer_sfd = accept(sfd, (struct sockaddr*)&peer_addr, &peer_addr_len);
  }
  close(sfd);

  char inet_addr[INET6_ADDRSTRLEN];
  inet_ntop(peer_addr.ss_family, get_in_addr((struct sockaddr*)&peer_addr),
            inet_addr, sizeof inet_addr);
  printf("Connected to %s\n", inet_addr);

  receive(peer_sfd, xdo, running);
  close(peer_sfd);
  printf("Connection closed\n");
}


void receive(int sfd, xdo_t* xdo, int* running)
{
  uint8_t buffer[RECVBUFSIZE];
  ssize_t nbytes = 0;
  char modifier_and_key[2*KEYSEQLEN+1];
  modifier_and_key[0] = '\0';

  size_t transfer = 0;
  while ((nbytes = recv(sfd, buffer+transfer, RECVBUFSIZE-transfer, 0))) {
    if (!*running) {
      printf("Shutting down...\n");
      return;
    }
    if (nbytes == -1) {
      perror("recv");
      return;
    }
    size_t unbytes = (size_t)nbytes;
    size_t processed = process_input(buffer, unbytes+transfer,
                                     modifier_and_key, xdo);
    if (processed < unbytes) {
      printf("receive: memmove\n");
      memmove(buffer, buffer+processed, (unbytes-processed)*sizeof(uint8_t));
      transfer = unbytes-processed;
    }
  }
}


size_t process_input(const uint8_t* buffer, size_t nbytes,
                     char* modifier_and_key, xdo_t* xdo)
{
  size_t processed_bytes = 0;
  while (nbytes > processed_bytes) {
    ssize_t processed = process_character(buffer + processed_bytes,
                                          nbytes - processed_bytes,
                                          modifier_and_key, xdo);
    if (processed == -1) {
      DEBUG_PRINT("Printing malformed input:\n");
      print_buffer(buffer + processed_bytes, nbytes - processed_bytes);
      processed = 1;
    }
    if (processed < -1) {
      return processed_bytes;
    }
    processed_bytes += (size_t)processed;
  }
  return processed_bytes;
}


ssize_t process_character(const uint8_t* buffer, size_t maxbytes,
                          char* modifier_and_key, xdo_t* xdo)
{
  ssize_t processed_bytes = 0;
  uint32_t unicode = 0;
  processed_bytes = utf8_to_unicode(buffer, &unicode, maxbytes);

  if (processed_bytes < 0) {
    DEBUG_PRINT("Received malformed (-1) or incomplete input (-n): %zd\n",
                processed_bytes);
    return processed_bytes;
  }
  if (processed_bytes <= 4) {
    if (unicode == HEARTBEAT) {
      DEBUG_PRINT("Received heartbeat\n");
      return processed_bytes;
    }
    char keysequence[KEYSEQLEN];
    if (unicode == 0x0a) {
      snprintf(keysequence, KEYSEQLEN, "Return");
    } else {
      snprintf(keysequence, KEYSEQLEN, "%#010x", unicode);
    }
    strcat(modifier_and_key, keysequence);
    DEBUG_PRINT("Sending '%s'\n", modifier_and_key);
    xdo_send_keysequence_window(xdo, CURRENTWINDOW, modifier_and_key, 12000);
    modifier_and_key[0] = '\0';
    return processed_bytes;
  }
  if (processed_bytes == 5) {
    uint16_t u1 = (uint16_t)(unicode>>13);
    uint16_t u2 = (uint16_t)(unicode & 0x1fff);
    int distX = ((u1 & 0x1000) ? -1 : 1) * (u1 & 0xfff);
    int distY = ((u2 & 0x1000) ? -1 : 1) * (u2 & 0xfff);
    DEBUG_PRINT("Mouse movement x: %d; y: %d\n", distX, distY);
    xdo_move_mouse_relative(xdo, distX, distY);
    return processed_bytes;
  }
  if (processed_bytes == 6) {
    if (unicode < MOUSECLICKSOFFSET + MOUSECLICKSLEN) {
      int mouse_button = mouse_clicks[unicode-MOUSECLICKSOFFSET];
      DEBUG_PRINT("Sending mouse click %d\n", mouse_button);
      xdo_click_window(xdo, CURRENTWINDOW, mouse_button);
    } else if (unicode < MODIFIERKEYSOFFSET + MODIFIERKEYSLEN) {
      const char* cur_modifier = modifier_keys[unicode-MODIFIERKEYSOFFSET];
      if (!strncmp(modifier_and_key, cur_modifier, strlen(cur_modifier))) {
        DEBUG_PRINT("Sending '%s'\n", cur_modifier);
        xdo_send_keysequence_window(xdo, CURRENTWINDOW, cur_modifier, 12000);
        modifier_and_key[0] = '\0';
      } else {
        strcpy(modifier_and_key, cur_modifier);
        strcat(modifier_and_key, "+");
      }
    } else if (unicode < SPECIALKEYSOFFSET + SPECIALKEYSLEN) {
      strcat(modifier_and_key, special_keys[unicode-SPECIALKEYSOFFSET]);
      DEBUG_PRINT("Sending '%s'\n", modifier_and_key);
      xdo_send_keysequence_window(xdo, CURRENTWINDOW, modifier_and_key, 12000);
      modifier_and_key[0] = '\0';
    }
    return processed_bytes;
  }
  DEBUG_PRINT("This must not happen: processed_bytes=%zd\n", processed_bytes);
  return -1;
}


void print_buffer(const uint8_t* buffer, size_t nbytes)
{
  DEBUG_PRINT("nbytes: %ld\n", nbytes);
  for (size_t i = 0; i < nbytes; ++i) {
    DEBUG_PRINT("0x%x ", buffer[i]);
  }
  DEBUG_PRINT("\n");
}
