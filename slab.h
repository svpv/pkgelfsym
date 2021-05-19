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

// A slab is a big chunk of memory to which objects are placed back to back.
// Objects are identified by their 32-bit offset (or "position") in the slab.
// Poistion 0 is reserved, and may serve as NULL.

#include "platform.h"

struct slab {
    uchar *base;
    uint32_t alloc;
    uint32_t fill;
};

void slab_init(struct slab *slab);
void slab_fini(struct slab *slab);

void slab_resize(struct slab *slab, size_t size);

static inline void slab_reserve(struct slab *slab, size_t size)
{
    size += slab->fill;
    if (unlikely(size > slab->alloc))
	slab_resize(slab, size);
}

static inline uint32_t slab_copy(struct slab *slab, const void *src, size_t size)
{
    uint32_t pos = slab->fill;
    memcpy(slab->base + pos, src, size);
    slab->fill += size;
    return pos;
}

static inline uint32_t slab_put(struct slab *slab, const void *src, size_t size)
{
    slab_reserve(slab, size);
    return slab_copy(slab, src, size);
}

static inline void *slab_get(const struct slab *slab, uint32_t pos)
{
    return slab->base + pos;
}
