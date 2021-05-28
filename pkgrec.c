#include <lz4.h>
#include <pfor16.h>
#include "pkgrec.h"
#include "slab.h"
#include "errexit.h"

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

static char *PkgRec_decodeFilenames(struct PkgRec *R, char *s, struct slab *slab, uint32_t pkgname)
{
    char s0[4096];
    size_t len0 = 0, lcp = 0;
    uint nfiles = R->h.n0files + 1;
    for (uint i = 0; i < nfiles; i++) {
	if (likely(i)) {
	    intptr_t delta = (int8_t) *s++;
	    if (unlikely(delta == -128))
		lcp = 0;
	    else {
		lcp += delta;
		assert(lcp <= len0);
	    }
	}
	size_t len = strlen(s);
	len0 = lcp + len;
	assert(len0 < sizeof s0);
	memcpy(s0 + lcp, s, len + 1);
	s += len + 1;
	R->fname[i] = slab_put(slab, s0, len0 + 1);
    }
    return s;
}

struct PkgRec *PkgRec_read(FILE *fp, struct slab *slab, uint32_t pkgname, uint32_t reclen)
{
    // read the header
    struct PkgRecHdr h;
    size_t nb = fread(&h, 1, sizeof h, fp);
    assert(nb == sizeof h);
    // alloc and read the record
    char *src0 = xmalloc(reclen);
    nb = fread(src0, 1, reclen, fp);
    assert(nb == reclen);
    char *src = src0;
    char *srcEnd = src0 + reclen;
    // alloc uncompressed buffer
    size_t nv16 = 2UL * (h.nsym0[0] + h.nsym0[1]) + h.nsym1 + h.nsym;
    size_t alloc = 2 * nv16 + h.frenclen + 4 * (h.n0files + 1);
    struct PkgRec *R = xmalloc(sizeof(*R) + alloc + 32);
    memcpy(&R->h, &h, sizeof R->h);
    // set up array pointers
    R->sym0 = (void *)(R->fname + h.n0files + 1);
    R->fsym = R->sym0 + h.nsym0[0] + h.nsym0[1];
    R->n0dup = R->fsym + h.nsym;
    R->T = (void *)(R->n0dup + h.nsym0[0] + h.nsym0[1] + h.nsym1);
    // decode 16-bit integers
    nb = pfor16dec(src, srcEnd - src, R->sym0, nv16);
    assert(nb > 0);
    src += nb;
    delta16dec(R->sym0, h.nsym0[0] + h.nsym0[1]);
    dmask16dec(R->fsym, h.nsym, mag16(h.n0files));
    dzag16dec(R->n0dup, h.nsym0[0] + h.nsym0[1] + h.nsym1);
    // decompress cdata/frenc segment
    int inb = LZ4_decompress_safe(src, R->T, srcEnd - src, h.frenclen);
    assert((uint) inb == h.frenclen);
    // process frenc filenames
    R->sym1frenc = PkgRec_decodeFilenames(R, R->T + h.nsym, slab, pkgname);
    // all done
    free(src0);
    return R;
}
