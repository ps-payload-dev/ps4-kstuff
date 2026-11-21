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

#include "patches.h"
#include <cerrno>

/*
 * Runs every patch that is not part of ps4-hen. The daily ps4-hen sync never
 * touches this directory, so add new custom patches here and register them in
 * this function.
 */
int kstuff_apply_custom(uintptr_t base, uint32_t version) {
  int error = 0;

  if (kstuff_apply_crash_log(base, version))
    error = errno ? errno : EIO;

  if (error) {
    errno = error;
    return -1;
  }

  return 0;
}
