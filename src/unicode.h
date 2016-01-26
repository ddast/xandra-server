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

#ifndef UNICODE_H
#define UNICODE_H

#include <xdo.h>

#define UNICODELEN 4

// Converts UTF-8 encoding (one to four byte) to Unicode codeposition.  Returns
// -1 on malformatted data.
int utf8_to_unicode(const unsigned char c[UNICODELEN]);

void send_keypress(int key, xdo_t* xdo);

#endif // UNICODE_H
