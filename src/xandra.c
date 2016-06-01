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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <xdo.h>

#include "server.h"
#include "unicode.h"

#define VERSION "0.4.3"

#define DEFAULTPORT "64296"

static int running = 1;

void print_help()
{
  printf("xandra v%s\n\n", VERSION);
  printf("Usage:\n");
  printf("xandra             start server on default port %s\n", DEFAULTPORT);
  printf("xandra [PORT]      start server on port [PORT]\n");
  printf("xandra --help      display this help and exit\n");
}

void sig_handler(int signo)
{
  if (signo == SIGINT) {
    running = 0;
  }
}

int main(int argc, char* argv[])
{
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sigemptyset(&sa.sa_mask);
  sa.sa_handler = &sig_handler;
  if (sigaction(SIGINT, &sa, NULL) == -1) {
    perror("sigaction");
    return EXIT_FAILURE;
  }

  char* port;
  switch (argc) {
  case 1:
    port = DEFAULTPORT;
    break;
  case 2:
    if (!strcmp(argv[1], "--help")) {
      print_help();
      return EXIT_SUCCESS;
    } else {
      port = argv[1];
    }
    break;
  default:
    print_help();
    return EXIT_FAILURE;
  }

  print_welcome();
  xdo_t* xdo = xdo_new(NULL);
  while (running) {
    int sfd = get_socket(port);
    accept_and_receive(sfd, xdo, &running);
  }
  xdo_free(xdo);
  return EXIT_SUCCESS;
}
