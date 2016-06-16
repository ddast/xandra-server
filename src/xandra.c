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
#include <ctype.h>
#include <signal.h>
#include <xdo.h>

#include "server.h"
#include "unicode.h"

#define VERSION "0.4.3"

#define DEFAULTPORT "64296"

static int running = 1;

// Prints version and usage information
void print_help(void);

// Sets the global variable running to 0 for SIGINT
void sig_handler(int signo);

// Returns 1 (0) if str is (not) a number
int str_is_number(const char* str);

void print_help(void)
{
  printf("xandra v%s\n", VERSION);
  printf("Usage: xandra [-4|-6] [PORT]\n");
  printf("Start xandra server on port PORT (default %s).\n\n", DEFAULTPORT);
  printf("  -4          use only IPv4\n");
  printf("  -6          use only IPv6\n");
  printf("  --help      display this help and exit\n");
}

void sig_handler(int signo)
{
  if (signo == SIGINT) {
    running = 0;
  }
}

int str_is_number(const char* str)
{
  for (; *str; ++str) {
    if (isdigit(*str) == 0) {
      return 0;
    }
  }
  return 1;
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

  if (argc > 3) {
    print_help();
    return EXIT_FAILURE;
  }

  const char* port = DEFAULTPORT;
  int protocol = 0;
  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "--help")) {
      print_help();
      return EXIT_SUCCESS;
    } else if (!strcmp(argv[i], "-4")) {
      protocol = 4;
    } else if (!strcmp(argv[i], "-6")) {
      protocol = 6;
    } else if (str_is_number(argv[i])) {
      port = argv[i];
    } else {
      print_help();
      return EXIT_FAILURE;
    }
  }

  print_welcome();
  xdo_t* xdo = xdo_new(NULL);
  while (running) {
    int sfd = get_socket(port, protocol);
    accept_and_receive(sfd, xdo, &running);
  }
  xdo_free(xdo);
  return EXIT_SUCCESS;
}
