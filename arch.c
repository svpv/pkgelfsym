// Copyright (c) 2019 Alexey Tourbin
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <elf.h>
#include <endian.h>
#include "arch.h"

const unsigned char arch_elfclass[NArch] = {
    [Arch_noarch]  = ELFCLASSNONE,
    [Arch_i686]    = ELFCLASS32,
    [Arch_amd64]   = ELFCLASS64,
    [Arch_arm64]   = ELFCLASS64,
    [Arch_ppc64le] = ELFCLASS64,
};

static bool elfpick_noarch(const void *eh_)
{
    (void) eh_;
    return 0;
}

static bool elfpick_i686(const void *eh_)
{
    const Elf32_Ehdr *eh = eh_;
    return  eh->e_ident[EI_DATA] == ELFDATA2LSB &&
	    eh->e_machine == htole16(EM_386);
}

static bool elfpick_amd64(const void *eh_)
{
    const Elf64_Ehdr *eh = eh_;
    return  eh->e_ident[EI_DATA] == ELFDATA2LSB &&
	    eh->e_machine == htole16(EM_X86_64);
}

static bool elfpick_arm64(const void *eh_)
{
    const Elf64_Ehdr *eh = eh_;
    return  eh->e_ident[EI_DATA] == ELFDATA2LSB &&
	    eh->e_machine == htole16(EM_AARCH64);
}

static bool elfpick_ppc64le(const void *eh_)
{
    const Elf64_Ehdr *eh = eh_;
    return  eh->e_ident[EI_DATA] == ELFDATA2LSB &&
	    eh->e_machine == htole16(EM_PPC64);
}

const arch_elfpick_t arch_elfpick[NArch] = {
    [Arch_noarch]  = elfpick_noarch,
    [Arch_i686]    = elfpick_i686,
    [Arch_amd64]   = elfpick_amd64,
    [Arch_arm64]   = elfpick_arm64,
    [Arch_ppc64le] = elfpick_ppc64le,
};
