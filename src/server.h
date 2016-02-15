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

#ifndef SERVER_H
#define SERVER_H

#include <xdo.h>

// Prints a welcome message including the hostname
void print_welcome();

// Binds and listens to a TCP socket on this machine.  Accepts both IP4 and IP6
// connections.  Returns the socket file descriptor.
int get_socket(char* port);

// Waits for client to connect on the socket sfd.  Accepts first peer and
// receives data until connection is closed by peer.  Uses xdo instance to send
// keystrokes and mouse movement.
void wait_and_receive(int sfd, xdo_t* xdo);

#endif // SERVER_H
