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

int utf8_to_unicode(const unsigned char* c, uint32_t* unicode)
{
  if (!(c[0] & 0x80)) {
    *unicode = c[0];
    return 1;
  }

  for (int i = 2; i <= 6; ++i) {
    if ((c[i-1] & 0xc0) != 0x80) {
      return -1;
    }
    if ((c[0] & (unsigned char)~(0xff>>(i+1))) == (unsigned char)~(0xff>>i)) {
      *unicode = (uint32_t)(c[0] & 0xff>>(i+1))<<(6*(i-1));
      for (int j = 1; j < i; ++j) {
        *unicode = *unicode | (uint32_t)(c[j] & 0x3f)<<(6*(i-j-1));
      }
      return i;
    }
  }

  return -1;
}
