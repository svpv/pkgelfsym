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

#include <stdbool.h>
#pragma GCC visibility push(hidden)

// We generally support the following architectures.  Architectures here
// are defined broadly, and apply to both packages and packaged binaries.
// An architecture is best thought of as an equivalence class: compatible
// packages/binaries (e.g. i386 and i686) must be of the same architecture,
// while incompatible ones must be assigned different architectures.
enum Arch {
    Arch_noarch,
    Arch_i686,
    Arch_amd64,
    Arch_arm64,
    Arch_ppc64le,
};

#define NArch 5

// The architecture of a package can be deduced from its filename.  We then
// need to pick ELF binaries from this package.  We further verify that the
// binaries agree with the architecture, e.g. we must skip 32-bit binaries
// inside 64-bit packages.  Thus the first thing to check is EI_CLASS.
extern const unsigned char arch_elfclass[NArch];

// We then load the header (either Elf32_Ehdr or Elf64_Ehdr, corresponding to
// EI_CLASS) and perform additional architecture-specific tests for byte order,
// e_machine, etc.
typedef bool (*arch_elfpick_t)(const void *eh);
extern const arch_elfpick_t arch_elfpick[NArch];

#pragma GCC visibility pop
