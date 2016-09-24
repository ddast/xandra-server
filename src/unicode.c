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

ssize_t utf8_to_unicode(const uint8_t* c, uint32_t* unicode, size_t maxbytes)
{
  if (!(c[0] & 0x80)) {
    *unicode = c[0];
    return 1;
  }

  for (size_t i = 2; i <= 6; ++i) {
    if ((c[0] & (uint8_t)~(0xff>>(i+1))) == (uint8_t)~(0xff>>i)) {
      if (i > maxbytes) {
        return -(ssize_t)i;
      }
      *unicode = (uint32_t)(c[0] & 0xff>>(i+1))<<(6*(i-1));
      for (size_t j = 1; j < i; ++j) {
        if ((c[j] & 0xc0) != 0x80) {
          return -1;
        }
        *unicode = *unicode | (uint32_t)(c[j] & 0x3f)<<(6*(i-j-1));
      }
      return (ssize_t)i;
    }
  }
  return -1;
}
