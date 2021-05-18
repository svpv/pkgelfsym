// Copyright (c) 2019, 2021 Alexey Tourbin
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

#include "slab.h"
#include "errexit.h"

void slab_init(struct slab *slab)
{
    // The initial size is about 47M, should force mmap.
    // It resizes well (by a factor of 1.5) up to almost 4G.
    slab->alloc = 12123 * 4096 - 24;
    slab->base = xmalloc(slab->alloc);
    // Poistion 0 is reserved, and may serve as NULL.
    slab->base[0] = 0;
    slab->fill = 1;
}

void slab_fini(struct slab *slab)
{
    free(slab->base), slab->base = NULL;
}

void slab_resize(struct slab *slab, size_t size)
{
    do {
	uint32_t alloc = slab->alloc;
	alloc += alloc / 2;
	alloc &= -4096;
	alloc -= 24;
	if (alloc < slab->alloc)
	    die("%s: out of 4GB", __func__);
	slab->alloc = alloc;
    } while (slab->alloc < size);
    slab->base = xrealloc(slab->base, slab->alloc);
}
