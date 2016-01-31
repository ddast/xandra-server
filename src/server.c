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

#include "server.h"
#include "unicode.h"

#define PORT "64296"
#define TIMEOUT 9
#define RECVBUFSIZE 4
#define KEYSTRLEN 7


// Returns the presentation IP4 or IP6 address stored in a sockaddr_storage.
void* get_in_addr(struct sockaddr* addr);

// Receives and forwards data until connection is closed by peer or times out.
void receive(int sfd, xdo_t* xdo);

// Processes the received input, depending on the content of buffer:
// 0:          heartbeat
// 1:          send backspace
// utf8 char:  send character using xdo
// else:       do nothing
void process_input(const unsigned char* buffer, xdo_t* xdo);


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


int get_socket()
{
  struct addrinfo hints, *result;
  memset(&hints, 0, sizeof hints);
  hints.ai_family   = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags    = AI_PASSIVE;

  int status = getaddrinfo(NULL, PORT, &hints, &result);
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
  while ((nbytes = recv(sfd, buffer, RECVBUFSIZE, 0)) != 0) {
    if (nbytes == -1) {
      perror("recv");
      return;
    }
    process_input(buffer, xdo);
  }
}


void process_input(const unsigned char* buffer, xdo_t* xdo)
{
  int unicode = utf8_to_unicode(buffer);
  if (unicode > 1) {
    char keystr[KEYSTRLEN];
    snprintf(keystr, KEYSTRLEN, "%#06x", unicode);
#ifdef DEBUG
    printf("Sending '%s'\n", keystr);
#endif
    xdo_send_keysequence_window(xdo, CURRENTWINDOW, keystr, 12000);
  } else if (unicode == 1) {
    xdo_send_keysequence_window(xdo, CURRENTWINDOW, "BackSpace", 12000);
  } else if (unicode == -1) {
    fprintf(stderr, "Received unknown character\n");
  }
#ifdef DEBUG
  else if (unicode == 0) {
    printf("Received heartbeat\n");
  }
#endif
}
