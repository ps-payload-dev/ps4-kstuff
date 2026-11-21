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

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "kstuff.h"
#include "ptrace.h"

static void
log_line(const char *line) {
  puts(line);
}

int
main(void) {
  kstuff_set_logger(log_line);

  if(patch_ps4_ptrace()) {
    printf("kstuff: cannot patch kernel ptrace (%s)\n", strerror(errno));
    return 1;
  }

  if(kstuff_apply()) {
    printf("kstuff: some patches were skipped (%s)\n", strerror(errno));
    return 1;
  }

  puts("kstuff: applied");
  return 0;
}
