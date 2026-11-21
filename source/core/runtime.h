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

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#define KSTUFF_PATCH_MAX 8

struct kstuff_patch {
  uintptr_t address;
  size_t length;
  uint8_t original[KSTUFF_PATCH_MAX];
  uint8_t replacement[KSTUFF_PATCH_MAX];
  int installed;
};

void kstuff_log(const char *format, ...) __attribute__((format(printf, 1, 2)));

pid_t kstuff_find_pid(const char *name);

int kstuff_patch_prepare(pid_t pid, kstuff_patch *patch);
int kstuff_patch_commit(pid_t pid, kstuff_patch *patch);
int kstuff_patch_restore(pid_t pid, kstuff_patch *patch);
