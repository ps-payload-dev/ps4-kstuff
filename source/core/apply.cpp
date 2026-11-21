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

#include "kstuff.h"
#include "patches.h"
#include "runtime.h"
#include <cerrno>
#include <cstring>
#include <ps4/kernel.h>

int kstuff_apply() {
  uint32_t firmware = kernel_get_fw_version();
  const auto *fw = kstuff_firmware_for(firmware);
  uintptr_t base = KERNEL_ADDRESS_IMAGE_BASE;

  if (!fw || (base >> 48) != 0xffff) {
    kstuff_log("[Kstuff] Unsupported firmware %#x; patches left unchanged\n",
               firmware);
    errno = ENOTSUP;
    return -1;
  }

  kstuff_log("[Kstuff] Preparing PS4HEN patches for firmware %u.%02u\n",
             fw->version / 100, fw->version % 100);

  int error = 0;
  bool shellcore_ready = !kstuff_check_shellcore(*fw);

  if (!shellcore_ready) {
    error = errno ? errno : EIO;
    kstuff_log(
        "[Kstuff] ShellCore does not match the %u.%02u table; ShellCore "
        "patches skipped\n",
        fw->version / 100, fw->version % 100);
  }

  // ps4-hen's installer applies exploit_fixes() before installing kpayload.
  if (kstuff_apply_exploit_fixes(base, fw->version)) {
    int failed = errno ? errno : EIO;
    kstuff_log("[Kstuff] ps4-hen exploit fixes incomplete: errno=%d (%s)\n",
               failed, strerror(failed));

    if (!error)
      error = failed;
  }

  if (kstuff_apply_custom(base, fw->version)) {
    int failed = errno ? errno : EIO;
    kstuff_log("[Kstuff] Custom patches incomplete: errno=%d (%s)\n", failed,
               strerror(failed));

    if (!error)
      error = failed;
  }

  if (kstuff_install_kernel_payload(base, *fw)) {
    int failed = errno ? errno : EIO;
    kstuff_log("[Kstuff] Kernel FSELF/FPKG hooks skipped: errno=%d (%s)\n",
               failed, strerror(failed));

    if (!error)
      error = failed;
  }

  if (shellcore_ready && kstuff_patch_shellcore(*fw)) {
    int failed = errno ? errno : EIO;
    kstuff_log("[Kstuff] ShellCore patches skipped: errno=%d (%s)\n", failed,
               strerror(failed));

    if (!error)
      error = failed;
  }

  kstuff_remote_play(fw->version);

  if (error) {
    errno = error;
    return -1;
  }

  kstuff_log("[Kstuff] Kernel, exploit, crash log, ShellCore and Remote Play "
             "patch stages complete\n");

  return 0;
}
