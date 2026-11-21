/* Copyright (C) 2026 ItsJokerZz

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING. If not, see
<http://www.gnu.org/licenses/>.
*/

#include "firmware.h"

static const struct kstuff_firmware firmwares[] = {
#include "firmware_table.inc"
};

const struct kstuff_firmware *kstuff_firmware_for(uint32_t sdk_version) {
  unsigned bcd = sdk_version >> 16, version = 0, multiplier = 1;

  for (int i = 0; i < 4; ++i, bcd >>= 4, multiplier *= 10) {
    unsigned digit = bcd & 15;

    if (digit > 9)
      return 0;

    version += digit * multiplier;
  }

  for (unsigned i = 0; i < sizeof(firmwares) / sizeof(*firmwares); ++i)
    if (firmwares[i].version == version)
      return &firmwares[i];

  return 0;
}
