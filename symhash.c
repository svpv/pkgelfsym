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

#include <unistd.h>
#include "symhash.h"
#include "platform.h"
#include "errexit.h"

#include <emmintrin.h>
#define Xmm __m128i
#define Add(x, a) *x = _mm_add_epi64(*x, *a)
#define Sub(x, a) *x = _mm_sub_epi64(*x, *a)
#define Xor(x, a) *x = _mm_xor_si128(*x, *a)

#define Shuf(x) *x = _mm_shuffle_epi32(*x, _MM_SHUFFLE(0, 1, 2, 3))

#define Mul13(x, y) _mm_mul_epu32(*x, _mm_shuffle_epi32(*y, _MM_SHUFFLE(2,3,0,1)))
#define Mul31(x, y) _mm_mul_epu32(*x, _mm_shuffle_epi32(*y, _MM_SHUFFLE(0,1,2,3)))

#define F0 Xor
#define F1 Sub
#define F2 Sub
#define F3 Add
#define F4 Sub
#define F5 Sub

static inline void update(Xmm *x, Xmm *y, Xmm *dx, Xmm *dy)
{
    F0(x, dx);
    F1(y, dy);
    Xmm mx = Mul13(x, y);
    Xmm my = Mul31(y, x);
    Shuf(y);
    F2(x, dy);
    F3(y, dx);
    Shuf(x);
    F4(x, &mx);
    F5(y, &my);
}

static inline uint64_t final(uint64_t *hi, Xmm *x, Xmm *y)
{
    Xmm mx = Mul31(x, y);
    Xmm my = Mul13(y, x);
    Shuf(x);
    Shuf(y);
    Add(x, &mx);
    Sub(y, &my);
    Xor(x, y);
    uint64_t ret[2];
    memcpy(ret, x, 16);
    *hi = ret[1];
    return ret[0];
}

static Xmm IV[2];

static __attribute__((constructor)) void initIV(void)
{
    int rc = getentropy(IV, sizeof IV);
    if (rc < 0)
	die("getentropy: %m");
}

uint64_t symhash(uint64_t *hi, char *str, size_t len)
{
    Xmm x = IV[0], y = IV[1];
    char *end = str + len;
    memset(end, 0, 32);
    do {
	Xmm dx, dy;
	memcpy(&dx, str + 0, 16);
	memcpy(&dy, str + 16, 16);
	str += 32;
	update(&x, &y, &dx, &dy);
    } while (str < end);
    return final(hi, &x, &y);
}
