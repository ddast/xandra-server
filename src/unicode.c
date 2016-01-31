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

#include "unicode.h"

int utf8_to_unicode(const unsigned char* c, int* unicode)
{
  if (!(c[0] & 0x80)) {
    *unicode = c[0];
    return 1;
  }

  if ((c[1] & 0xc0) != 0x80) {
    *unicode = -1;
    return 1;
  }

  if ((c[0] & 0xe0) == 0xc0) {
    *unicode = (c[0] & 0x1f)<<6 |
               (c[1] & 0x3f);
    return 2;
  }

  if ((c[2] & 0xc0) != 0x80) {
    *unicode = -1;
    return 2;
  }

  if ((c[0] & 0xf0) == 0xe0) {
    *unicode = (c[0] & 0x0f)<<12 |
               (c[1] & 0x3f)<<6  |
               (c[2] & 0x3f);
    return 3;
  }

  if ((c[3] & 0xc0) != 0x80) {
    *unicode = -1;
    return 3;
  }

  if ((c[0] & 0xf8) == 0xf0) {
    *unicode = (c[0] & 0x0f)<<18 |
               (c[1] & 0x3f)<<12 |
               (c[2] & 0x3f)<<6  |
               (c[3] & 0x3f);
    return 4;
  }

  return -1;
}
