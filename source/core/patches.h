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

#include "firmware.h"

int kstuff_install_kernel_payload(uintptr_t base, const kstuff_firmware &fw);
int kstuff_check_exploit_fixes(uintptr_t base, uint32_t version,
                               bool *already_applied);
int kstuff_apply_exploit_fixes(uintptr_t base, uint32_t version);
int kstuff_apply_custom(uintptr_t base, uint32_t version);
int kstuff_apply_crash_log(uintptr_t base, uint32_t version);
int kstuff_check_shellcore(const kstuff_firmware &fw);
int kstuff_patch_shellcore(const kstuff_firmware &fw);
void kstuff_remote_play(uint32_t version);
