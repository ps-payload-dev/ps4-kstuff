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

#include <elf.h>

__asm__(".section .rodata\n"
        ".align 16\n"
        "kstuff_elf:\n"
        ".incbin \"" KSTUFF_ELF "\"\n"
        ".previous\n");
extern const unsigned char kstuff_elf[];

#define SYS_MUNMAP 73
#define SYS_MMAP 477

#define PROT_RWX 7
#define MAP_PRIVATE_ANON 0x1002
#define PAGE 0x4000UL

static long
syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
  register long r10 __asm__("r10") = a4;
  register long r8 __asm__("r8") = a5;
  register long r9 __asm__("r9") = a6;
  unsigned long ret;
  char failed;

  __asm__ __volatile__("syscall"
                       : "=a"(ret), "=@ccc"(failed), "+r"(r10), "+r"(r8),
                         "+r"(r9)
                       : "a"(n), "D"(a1), "S"(a2), "d"(a3)
                       : "rcx", "r11", "memory");
  return failed ? -(long)ret : (long)ret;
}

static void
copy(unsigned char *dst, const unsigned char *src, unsigned long n) {
  __asm__ __volatile__("rep movsb" : "+D"(dst), "+S"(src), "+c"(n) : : "memory");
}

static void
run(const unsigned char *elf) {
  const Elf64_Ehdr *eh = (const Elf64_Ehdr *)elf;
  const Elf64_Phdr *ph = (const Elf64_Phdr *)(elf + eh->e_phoff);
  const Elf64_Shdr *sh = (const Elf64_Shdr *)(elf + eh->e_shoff);
  unsigned long lo = ~0UL;
  unsigned long hi = 0;
  unsigned char *img;
  void (*entry)(void);

  if(eh->e_ident[EI_MAG0] != ELFMAG0 || eh->e_ident[EI_MAG1] != ELFMAG1
     || eh->e_ident[EI_MAG2] != ELFMAG2 || eh->e_ident[EI_MAG3] != ELFMAG3) {
    return;
  }

  for(int i = 0; i < eh->e_phnum; i++) {
    if(ph[i].p_type != PT_LOAD) {
      continue;
    }
    if(ph[i].p_vaddr < lo) {
      lo = ph[i].p_vaddr;
    }
    if(ph[i].p_vaddr + ph[i].p_memsz > hi) {
      hi = ph[i].p_vaddr + ph[i].p_memsz;
    }
  }

  lo &= ~(PAGE - 1);
  hi = (hi + PAGE - 1) & ~(PAGE - 1);

  img = (unsigned char *)syscall6(SYS_MMAP, 0, hi - lo, PROT_RWX,
                                  MAP_PRIVATE_ANON, -1, 0);
  if((long)img < 0 && (long)img > -4096) {
    return;
  }

  for(int i = 0; i < eh->e_phnum; i++) {
    if(ph[i].p_type == PT_LOAD && ph[i].p_filesz) {
      copy(img + ph[i].p_vaddr - lo, elf + ph[i].p_offset, ph[i].p_filesz);
    }
  }

  for(int i = 0; i < eh->e_shnum; i++) {
    if(sh[i].sh_type != SHT_RELA) {
      continue;
    }

    const Elf64_Rela *rela = (const Elf64_Rela *)(elf + sh[i].sh_offset);
    unsigned long count = sh[i].sh_size / sizeof(Elf64_Rela);

    for(unsigned long j = 0; j < count; j++) {
      if(ELF64_R_TYPE(rela[j].r_info) == R_X86_64_RELATIVE) {
        unsigned long value = (unsigned long)img + rela[j].r_addend - lo;
        copy(img + rela[j].r_offset - lo, (const unsigned char *)&value,
             sizeof(value));
      }
    }
  }

  entry = (void (*)(void))(img + eh->e_entry - lo);
  entry();

  syscall6(SYS_MUNMAP, (long)img, hi - lo, 0, 0, 0, 0);
}

int
_start(void) {
  run(kstuff_elf);
  return 0;
}
