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

OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(kstuff_kernel_start)
SECTIONS {
  . = 0;
  .payload : {
    KEEP(*(.payload_header))
    *(.payload_code .text .text.*)
    *(.payload_rdata .rodata .rodata.*)
    *(.payload_data .data .data.*)
    *(.payload_bss .bss .bss.* COMMON)
    BYTE(0)
    __image_end = .;
  }
  ASSERT(SIZEOF(.payload) <= 0x10000, "kstuff exceeds the reserved resident region")
  /DISCARD/ : { *(.comment .note* .eh_frame* .interp .gnu*) }
}
