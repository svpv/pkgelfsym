// Copyright (c) 2016, 2018, 2021 Alexey Tourbin
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

#include <stdio.h>
#include <stdlib.h>
#include "platform.h"

#define PROG "pkgelfsym"
#define warn(fmt, args...) fprintf(stderr, PROG ": " fmt "\n", ##args)
#define die(fmt, args...) warn(fmt, ##args), exit(128) // like git

static inline void *xmalloc_(const char *func, size_t n)
{
    void *p = malloc(n);
    if (unlikely(!p))
	die("%s: %m", func);
    return p;
}

static inline void *xrealloc_(const char *func, void *p, size_t n)
{
    p = realloc(p, n);
    if (unlikely(!p))
	die("%s: %m", func);
    return p;
}

#define xmalloc(n) xmalloc_(__func__, n)
#define xrealloc(p, n) xrealloc_(__func__, p, n)
