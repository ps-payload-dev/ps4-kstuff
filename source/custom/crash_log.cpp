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
#include "runtime.h"
#include <cerrno>
#include <cstring>
#include <ps4/kernel.h>

struct kstuff_crash_log_site {
  uint32_t version;
  uint32_t offset;
};

static const kstuff_crash_log_site crash_log_sites[] = {
    {474, 0x169790},   {500, 0x11730},    {501, 0x11730},   {503, 0x11730},
    {505, 0x11730},    {507, 0x11730},    {550, 0x1b4920},  {553, 0x1b4820},
    {555, 0x1b4b90},   {556, 0x459420},   {600, 0x459420},  {602, 0x459420},
    {620, 0x459440},   {650, 0x233820},   {651, 0x233820},  {670, 0x233bd0},
    {671, 0x233bd0},   {672, 0x233bd0},   {700, 0x1cb880},  {701, 0x1cb880},
    {702, 0x1cb880},   {750, 0x364cd0},   {751, 0x364cd0},  {755, 0x364cd0},
    {800, 0x1d5710},   {801, 0x1d5710},   {803, 0x1d5710},  {850, 0x2935e0},
    {852, 0x2935e0},   {900, 0x8bc20},    {903, 0x8bc20},   {904, 0x8bc20},
    {950, 0x32590},    {951, 0x32590},    {960, 0x32590},   {1000, 0xa5c60},
    {1001, 0xa5c60},   {1050, 0x1f4470},  {1070, 0x1f4470}, {1071, 0x1f4470},
    {1100, 0x3d0de0},  {1102, 0x3d0e00},  {1150, 0x3b2a90}, {1152, 0x3b2a90},
    {1200, 0x3b2cd0},  {1202, 0x3b2cd0},  {1250, 0x3b2d10}, {1252, 0x3b2d10},
    {1300, 0x3b2d30},  {1302, 0x3b2d40},  {1304, 0x3b2d40}, {1350, 0x3b3180},
    {1352, 0x3b3580},  {1400, 0x3b3830},  {1402, 0x3b3ba0},
};

static const uint8_t crash_log_original[] = {0x55, 0x48, 0x89, 0xe5};
static const uint8_t crash_log_patched[] = {0xb0, 0x01, 0xc3, 0x90};
static const uint8_t crash_log_returns_one[] = {0x48, 0xc7, 0xc0, 0x01,
                                                0x00, 0x00, 0x00, 0xc3};
static const uint8_t crash_log_tail[] = {0x31, 0xc9, 0x83, 0xf8, 0x01, 0x0f,
                                         0x94, 0xc1, 0x89, 0xc8, 0x5d, 0xc3};

int kstuff_apply_crash_log(uintptr_t base, uint32_t version) {
  const kstuff_crash_log_site *site = nullptr;

  for (const auto &entry : crash_log_sites) {
    if (entry.version == version) {
      site = &entry;
      break;
    }
  }

  if (!site) {
    kstuff_log("[Kstuff] No crash log patch for firmware %u\n", version);
    return 0;
  }

  uint8_t bytes[sizeof(crash_log_original) + 5 + sizeof(crash_log_tail)];

  if (kernel_copyout(base + site->offset, bytes, sizeof(bytes))) {
    int failed = errno ? errno : EIO;
    kstuff_log("[Kstuff] Crash log site +%#x unreadable: errno=%d (%s)\n",
               site->offset, failed, strerror(failed));
    errno = failed;

    return -1;
  }

  bool original = !memcmp(bytes, crash_log_original, sizeof(crash_log_original));
  bool patched = !memcmp(bytes, crash_log_patched, sizeof(crash_log_patched)) ||
                 !memcmp(bytes, crash_log_returns_one,
                         sizeof(crash_log_returns_one));
  bool tail_ok = !memcmp(bytes + 9, crash_log_tail, sizeof(crash_log_tail));

  if (!tail_ok || (!original && !patched) || (original && bytes[4] != 0xe8)) {
    kstuff_log("[Kstuff] Crash log site +%#x does not hold the kit check; "
               "patch skipped; bytes %02x %02x %02x %02x %02x %02x %02x %02x "
               "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x "
               "%02x\n",
               site->offset, bytes[0], bytes[1], bytes[2], bytes[3], bytes[4],
               bytes[5], bytes[6], bytes[7], bytes[8], bytes[9], bytes[10],
               bytes[11], bytes[12], bytes[13], bytes[14], bytes[15],
               bytes[16], bytes[17], bytes[18], bytes[19], bytes[20]);
    errno = ENOEXEC;

    return -1;
  }

  if (patched) {
    kstuff_log("[Kstuff] Kernel crash log patch already applied\n");

    return 0;
  }

  errno = 0;

  if (kernel_copyin(crash_log_patched, base + site->offset,
                    sizeof(crash_log_patched))) {
    int failed = errno ? errno : EIO;
    kstuff_log("[Kstuff] Crash log patch failed at +%#x: errno=%d (%s)\n",
               site->offset, failed, strerror(failed));
    errno = failed;

    return -1;
  }

  kstuff_log("[Kstuff] Kernel crash log patch applied at +%#x\n", site->offset);

  return 0;
}
