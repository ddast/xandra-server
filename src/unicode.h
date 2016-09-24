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

// Converts one UTF-8 encoded character beginning at c (one to six bytes) to
// its Unicode codeposition and stores it in unicode.  Processes at most
// maxbytes bytes of the buffer c.
// Return value:
//   on success, the amount of processed bytes
//   -1 on malformed data
//   -n if first byte indicates an n-byte character, but n > maxbytes
ssize_t utf8_to_unicode(const uint8_t* c, uint32_t* unicode, size_t maxbytes);

#endif // UNICODE_H
