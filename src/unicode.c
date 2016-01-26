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

#define KEYSTRLEN 7

int utf8_to_unicode(const unsigned char c[UNICODELEN])
{
  if (!(c[0] & 0x80)) {
    // one byte
    return c[0];
  }

  if ((c[1] & 0xc0) != 0x80) {
    return -1;
  }

  if ((c[0] & 0xe0) == 0xc0) {
    // two bytes
    return (c[0] & 0x1f)<<6 |
           (c[1] & 0x3f);
  }

  if ((c[2] & 0xc0) != 0x80) {
    return -1;
  }

  if ((c[0] & 0xf0) == 0xe0) {
    // three bytes
    return (c[0] & 0x0f)<<12 |
           (c[1] & 0x3f)<<6  |
           (c[2] & 0x3f);
  }

  if ((c[3] & 0xc0) != 0x80) {
    return -1;
  }

  if ((c[0] & 0xf8) == 0xf0) {
    // four bytes
    return (c[0] & 0x0f)<<18 |
           (c[1] & 0x3f)<<12 |
           (c[2] & 0x3f)<<6  |
           (c[3] & 0x3f);
  }

  return -1;
}

void send_keypress(int key, xdo_t* xdo)
{
  char keystr[KEYSTRLEN];
  snprintf(keystr, KEYSTRLEN, "%#06x", key);

  printf("Sending '%s'\n", keystr);

  Window window = 0;
  int ret = xdo_get_focused_window_sane(xdo, &window);
  if (ret) {
    fprintf(stderr, "xdo_get_focused_window_sane");
  } else {
    xdo_send_keysequence_window(xdo, window, keystr, 12000);
  }
  // the above solution seems to work better with the locale keymap than
  // xdo_send_keysequence_window(xdo, CURRENTWINDOW, keystr, 12000);
}
