#include <stdio.h>
#include <unistd.h>
#include <lz4hc.h>
#include <pfor16.h>
#include "platform.h"
#include "pkgrec.h"

static uint read16v(uint16_t *v)
{
    uint16_t *v0 = v;
    while (1) {
	unsigned u;
	if (scanf("%x", &u) != 1)
	    break;
	assert(u <= UINT16_MAX);
	*v++ = u;
    }
    return v - v0;
}

static void read16v2(uint16_t *v, uint *n0p, uint *n1p)
{
    uint n0 = 0, n1 = 0;
    while (1) {
	uint u;
	if (scanf("%x", &u) != 1)
	    break;
	*v++ = u;
	if (u >= 0x10000) {
	    assert(u <= 0x1ffff);
	    n1++;
	}
	else {
	    assert(n1 == 0);
	    n0++;
	}
    }
    *n0p = n0, *n1p = n1;
}

static uint max16v(const uint16_t *v, uint n)
{
    uint max = 0;
    const uint16_t *end = v + n;
    while (v < end) {
	if (max < *v)
	    max = *v;
	v++;
    }
    return max;
}

static inline unsigned log2i(unsigned x)
{
#ifdef __LZCNT__
    return 31 - __builtin_clz(x);
#endif
    return 31 ^ __builtin_clz(x);
}

// The magnitude of a 16-bit integer: the smallest m such that the integer
// is representable with m bits.
static inline unsigned mag16(uint16_t x)
{
    return log2i(2 * x + 1);
}

// Usage: $0 pkg.sym0i pkg.fsym pkg.n0dup pkg.cdata
int main(int argc, char **argv)
{
    assert(argc == 1 + 4);

    struct PkgRecHdr h = {};
    static uint16_t vbuf[32<<20];
    uint16_t *v = vbuf;
    {
	// sym0i
	stdin = freopen(argv[1], "r", stdin);
	assert(stdin);
	uint sym0n0, sym0n1;
	read16v2(v, &sym0n0, &sym0n1);
	h.nsym0[0] = sym0n0, h.nsym0[1] = sym0n1;
	uint nsym0 = sym0n0 + sym0n1;
	delta16enc(v, nsym0);
	v += nsym0;
    }
    {
	// fsym
	stdin = freopen(argv[2], "r", stdin);
	assert(stdin);
	h.nsym = read16v(v);
	h.n0files = max16v(v, h.nsym);
	dmask16enc(v, h.nsym, mag16(h.n0files));
	v += h.nsym;
    }
    {
	// n0dup
	stdin = freopen(argv[3], "r", stdin);
	assert(stdin);
	uint n = read16v(v);
	dzag16enc(v, n);
	v += n;
	assert(n >= h.nsym0[0] + h.nsym0[1]);
	h.nsym1 = n - h.nsym0[0] - h.nsym0[1];
    }
    assert(!isatty(1));
    static char obuf[64<<20];
    char *out = obuf, *oend = obuf + sizeof obuf;
    {
	//  pfor16
	size_t n = pfor16enc(vbuf, v - vbuf, out);
	assert(n > 0);
	out += n;
    }
    {
	// cdata
	stdin = freopen(argv[4], "r", stdin);
	assert(stdin);
	size_t n = fread(out, 1, oend - out, stdin);
	assert(n > 0 && n < oend - out);

	// complete and write the header
	h.frenclen = n;
	fwrite(&h, 1, sizeof h, stdout);
	// write pfor16
	fwrite(obuf, 1, out - obuf, stdout);

	// resume fenc
	char *zout = out + n;
	int zlen = LZ4_compress_HC(out, zout, n, oend - zout, 7);

	assert(zlen > 0);
	fwrite(zout, 1, zlen, stdout);
    }
    return 0;
}
