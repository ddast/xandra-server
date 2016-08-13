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

#include <stdint.h>

// Converts one UTF-8 encoded character beginning at c (one to four byte) to
// its Unicode codeposition and stores it in unicode.  Returns the amount of
// bytes that have been processed.  Returns -1 on malformatted data.
int utf8_to_unicode(const unsigned char* c, int32_t* unicode);

#endif // UNICODE_H
