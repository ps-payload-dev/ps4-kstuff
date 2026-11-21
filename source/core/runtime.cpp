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

#include "runtime.h"
#include "kstuff.h"
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ps4/klog.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <vector>

extern "C" {
#include "ptrace.h"
}

static void (*logger)(const char *line);
static int (*patch_hook)(pid_t pid, uintptr_t address, size_t length,
                         const uint8_t *original);

void kstuff_set_logger(void (*sink)(const char *line)) { logger = sink; }

void kstuff_set_patch_hook(int (*hook)(pid_t pid, uintptr_t address,
                                       size_t length,
                                       const uint8_t *original)) {
  patch_hook = hook;
}

void kstuff_log(const char *format, ...) {
  int saved = errno;
  char line[1024];
  va_list args;

  va_start(args, format);
  vsnprintf(line, sizeof(line), format, args);
  va_end(args);

  if (logger)
    logger(line);
  else
    klog_printf("%s", line);

  errno = saved;
}

pid_t kstuff_find_pid(const char *name) {
  int mib[] = {CTL_KERN, KERN_PROC, KERN_PROC_PROC, 0};
  size_t size = 0;

  if (!name || sysctl(mib, 4, nullptr, &size, nullptr, 0))
    return -1;

  std::vector<uint8_t> buffer(size + sizeof(kinfo_proc) * 16);

  size = buffer.size();

  if (sysctl(mib, 4, buffer.data(), &size, nullptr, 0))
    return -1;

  for (size_t offset = 0; offset + sizeof(kinfo_proc) <= size;) {
    const auto *ki =
        reinterpret_cast<const kinfo_proc *>(buffer.data() + offset);
    size_t record_size = ki->ki_structsize > 0 ? (size_t)ki->ki_structsize : 0;

    if (record_size < sizeof(kinfo_proc) || record_size > size - offset)
      break;

    if (ki->ki_stat != 5 &&
        (!strncmp(ki->ki_comm, name, sizeof(ki->ki_comm)) ||
         !strncmp(ki->ki_tdname, name, sizeof(ki->ki_tdname))))
      return ki->ki_pid;

    offset += record_size;
  }

  return -1;
}

int kstuff_patch_prepare(pid_t pid, kstuff_patch *patch) {
  if (!patch || !patch->address || !patch->length ||
      patch->length > KSTUFF_PATCH_MAX || patch->installed) {
    errno = EINVAL;
    return -1;
  }

  memset(patch->original, 0, sizeof(patch->original));

  if (pt_copyout(pid, patch->address, patch->original, patch->length))
    return -1;

  if (patch_hook &&
      patch_hook(pid, patch->address, patch->length, patch->original)) {
    if (!errno)
      errno = EIO;

    return -1;
  }

  return 0;
}

int kstuff_patch_commit(pid_t pid, kstuff_patch *patch) {
  if (!patch || !patch->address || !patch->length ||
      patch->length > KSTUFF_PATCH_MAX || patch->installed) {
    errno = EINVAL;
    return -1;
  }

  uint8_t current[KSTUFF_PATCH_MAX];
  size_t length = patch->length;

  if (pt_copyout(pid, patch->address, current, length))
    return -1;

  if (memcmp(current, patch->original, length)) {
    errno = EBUSY;
    return -1;
  }

  patch->installed = 1;

  if (!pt_copyin(pid, patch->replacement, patch->address, length) &&
      !pt_copyout(pid, patch->address, current, length) &&
      !memcmp(current, patch->replacement, length))
    return 0;

  int error = errno ? errno : EIO;

  if (!pt_copyin(pid, patch->original, patch->address, length) &&
      !pt_copyout(pid, patch->address, current, length) &&
      !memcmp(current, patch->original, length))
    patch->installed = 0;

  errno = error;

  return -1;
}

int kstuff_patch_restore(pid_t pid, kstuff_patch *patch) {
  if (!patch || !patch->address || !patch->length ||
      patch->length > KSTUFF_PATCH_MAX) {
    errno = EINVAL;
    return -1;
  }

  uint8_t current[KSTUFF_PATCH_MAX];
  size_t length = patch->length;

  if (pt_copyout(pid, patch->address, current, length))
    return -1;

  if (!memcmp(current, patch->original, length)) {
    patch->installed = 0;
    return 0;
  }

  if (memcmp(current, patch->replacement, length)) {
    errno = EBUSY;
    return -1;
  }

  if (pt_copyin(pid, patch->original, patch->address, length) ||
      pt_copyout(pid, patch->address, current, length))
    return -1;

  if (memcmp(current, patch->original, length)) {
    errno = EIO;
    return -1;
  }

  patch->installed = 0;

  return 0;
}
