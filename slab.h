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

// A slab is a big chunk of memory to which objects are placed back to back.
// Objects are identified by their 32-bit offset (or "position") in the slab.

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct slab {
    char *base;
    uint32_t alloc;
    uint32_t fill;
};

static inline void slab_init(struct slab *slab)
{
    // The initial size is about 47M, should force mmap.
    // It resizes well (by a factor of 1.5) up to almost 4G.
    slab->alloc = 12123 * 4096 - 24;
    slab->base = malloc(slab->alloc);
    assert(slab->base);
    // Poistion 0 is reserved, and may serve as NULL.
    slab->base[0] = '\0';
    slab->fill = 1;
}

static inline void slab_fini(struct slab *slab)
{
    free(slab->base), slab->base = NULL;
}

static void slab_resize(struct slab *slab)
{
    size_t alloc = slab->alloc;
    alloc += alloc / 2;
    alloc &= -4096;
    alloc -= 24;
    assert(alloc <= UINT32_MAX);
    slab->alloc = alloc;
    slab->base = realloc(slab->base, slab->alloc);
    assert(slab->base);
}

static inline void slab_reserve(struct slab *slab, size_t size)
{
    size_t fill = slab->fill + size;
    if (fill > slab->alloc)
	slab_resize(slab);
}

static inline uint32_t slab_copy(struct slab *slab, void *src, size_t size)
{
    assert(size > 0);
    uint32_t pos = slab->fill;
    size_t fill = slab->fill + size;
    assert(fill <= slab->alloc);
    slab->fill = fill;
    memcpy(slab->base + pos, src, size);
    return pos;
}

static inline uint32_t slab_put(struct slab *slab, void *src, size_t size)
{
    slab_reserve(slab, size);
    return slab_copy(slab, src, size);
}

static inline void *slab_get(struct slab *slab, uint32_t pos)
{
    assert(pos < slab->fill);
    return slab->base + pos;
}
