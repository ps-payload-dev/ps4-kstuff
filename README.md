# ps4-kstuff
A library of patches for PS4 derived from the work of [PS4-HEN](https://github.com/Scene-Collective/ps4-hen) by Scene Collective

Installs the FSELF/FPKG kernel hooks and kernel patches, patches SceShellCore so packages and system versions are accepted, enables the kernel crash log, and applies the Remote Play patches. The firmwares in `source/generated/firmware_table.inc` are supported (4.74 to 14.00 at the moment); anything else is left untouched.

## Layout

| Path | What it is |
| --- | --- |
| `headers/` | Public API (`kstuff.h`) and the shared firmware data types |
| `source/core/` | Patch coordinator (`apply.cpp`), kernel payload installer, firmware lookup, runtime helpers and the internal `patches.h` |
| `source/hen/` | Patches ported from ps4-hen: installer exploit fixes, ShellCore and Remote Play |
| `source/custom/` | Patches that are not part of ps4-hen, such as the kernel crash log. The ps4-hen sync never touches this folder |
| `source/generated/` | Per-firmware tables generated from ps4-hen by `tools/generate_tables.py` |
| `kernel/` | Kernel payload: FSELF/FPKG hooks, built with the host `gcc` into `kstuff-kernel.bin` |
| `payload/` | The runnable payload: `runner.c` (ELF entry) and `bootstrap.c` with `bootstrap.x` (BIN loader) |
| `vendor/ps4-hen/` | Kernel helper headers copied unmodified from [Scene-Collective/ps4-hen](https://github.com/Scene-Collective/ps4-hen), and the `UPSTREAM` file that records the synced commit |
| `tools/` | `sync_hen.sh` pulls ps4-hen and `generate_tables.py` builds the tables |
| `.github/` | The daily ps4-hen sync workflow and the issue templates |

## API

```c
#include "kstuff.h"

void kstuff_set_logger(void (*sink)(const char *line));
void kstuff_set_patch_hook(int (*hook)(pid_t pid, uintptr_t address,
                                       size_t length, const uint8_t *original));
int kstuff_apply(void);
int kstuff_reapply_shellcore(void);
```

`kstuff_set_logger` is optional; without it, messages go to klog. `kstuff_set_patch_hook` is optional too: the hook is called with the address and original bytes of every process patch just before it is written, and a non-zero return cancels that patch. `kstuff_apply` returns non-zero with `errno` set when something was skipped. The static library is meant to be linked together with [lib-ptrace][ptrace].

## Building

Needs the [payload SDK][sdk] (`PS4_PAYLOAD_SDK=/opt/ps-payload-sdk/ps4`), a host `gcc`, `xxd` and a [lib-ptrace][ptrace] checkout.

```sh
make PTRACE_DIR=../lib-ptrace BUILD=build
```

| Output | Path |
| --- | --- |
| Library | `$(BUILD)/lib/libkstuff.a` |
| Runnable ELF | `$(BUILD)/bin/kstuff.elf` |
| Runnable BIN | `$(BUILD)/bin/kstuff.bin` |
| Kernel payload | `$(BUILD)/bin/kstuff-kernel.bin` and `.elf` |

`make` builds all of the above. Build one at a time with `make lib`, `make elf`,
`make bin` or `make kernel`. The ELF and BIN need a built [lib-ptrace][ptrace]
archive, which `make` builds for you in `PTRACE_DIR`. `make kernel` builds only
the kernel payload and needs nothing but a host `gcc`.

## Running

`kstuff.elf` is a standalone payload. It patches the kernel ptrace checks with
`patch_ps4_ptrace()`, then calls `kstuff_apply()` and prints each step. Send it to
an ELF loader running on the console, for example on port 9021:

```sh
PS4_HOST=ps4
nc -q0 "$PS4_HOST" 9021 < build/bin/kstuff.elf
```

`kstuff.bin` is the same payload for BIN loaders. It is a small position
independent bootstrap with `kstuff.elf` embedded, whose entry point is at byte
zero, so it can be sent to a BIN loader on port 9020:

```sh
nc -q0 "$PS4_HOST" 9020 < build/bin/kstuff.bin
```

To use kstuff from your own payload instead, link `libkstuff.a` and
`libptrace.a` and call the API above.

## Syncing with ps4-hen

```sh
sh tools/sync_hen.sh            # ps4-hen HEAD
sh tools/sync_hen.sh <ref>      # any branch, tag or commit of ps4-hen
```

`sh update.sh` still works and runs the same script. The script fetches that
revision, overwrites the headers in `vendor/ps4-hen/`, regenerates the firmware
and Remote Play tables from `kpayload/`, and imports every firmware-specific
write in the installer's `exploit_fixes()` from `installer/source/kpayloads.c`
into `source/generated/`. It stops before changing anything if required offsets,
installer values or `exploit_fixes()` records are missing or have an unrecognized
form. `kstuff_apply()` runs the ps4-hen cleanup patches before installing the
kernel payload, following HEN's installer order, and a ShellCore mismatch skips
only the ShellCore stage.

### Daily sync

The workflow `.github/workflows/sync-ps4-hen.yml` runs every day and can be
started by hand. It pulls the latest ps4-hen, builds the library, ELF, BIN and
kernel payload, and then:

- **Build passes:** the new offsets, exploit fixes and headers are committed to
  the default branch automatically.
- **Build fails:** nothing is merged. The sync is pushed to the `sync-ps4-hen`
  branch and a pull request is opened for a manual look.
- **Runtime code changed:** ps4-hen's `kpayload/source` and `installer/source`
  logic is not copied blindly. When it changes, the workflow opens an issue that
  lists the files so they can be ported by hand into `kernel/` or `source/hen/`.

The sync only ever writes `vendor/ps4-hen/` and `source/generated/`, and the
workflow fails if it would touch anything else. That is what keeps your custom
patches safe.

### Custom patches

Put patches that are not part of ps4-hen in `source/custom/` and call them from
`kstuff_apply_custom()` in `source/custom/custom.cpp`. They run as one stage of
`kstuff_apply()` and are never overwritten by the sync. A new ps4-hen firmware
that a custom patch has no entry for is logged and skipped for that patch only.

## Contributing

[Report a bug or request a feature][issues] with the issue templates. For larger
changes, open an issue before starting work so the approach can be discussed.

[sdk]: https://github.com/ps-payload-dev/sdk
[ptrace]: https://github.com/ps-payload-dev/lib-ptrace
[issues]: https://github.com/ItsJokerZz/ps4-kstuff/issues/new/choose
