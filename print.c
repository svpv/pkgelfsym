#include <unistd.h>
#include <sys/stat.h>
#include "pkgrec.h"
#include "slab.h"
#include "errexit.h"

char **load_topsym(void)
{
    FILE *fp = fopen("topsym.list", "r");
    assert(fp);
    struct stat st;
    int rc = fstat(fileno(fp), &st);
    assert(rc == 0);
    assert(st.st_size > 0);
    char **topsym = xmalloc((128<<10) * sizeof(*topsym) + st.st_size);
    char *s = (char *)(topsym + (128<<10));
    size_t nb = fread(s, 1, st.st_size, fp);
    assert(nb == st.st_size);
    fclose(fp);
    for (size_t i = 0; i < (128<<10); i++) {
	topsym[i] = s;
	s = strchr(s, '\n');
	assert(s);
	*s++ = '\0';
    }
    return topsym;
}

uint16_t *print_sym0(struct PkgRec *R, struct slab *slab, char **topsym)
{
    uint16_t *sym0 = R->sym0;
    uint16_t *n0dup = R->n0dup;
    uint16_t *fsym = R->fsym;
    char *T = R->T;
    uint nsym = R->h.nsym0[0];
    for (uint k = 0; k < 2; k++) {
	if (k) {
	    sym0 += nsym, n0dup += nsym;
	    nsym = R->h.nsym0[1];
	    topsym += 64<<10;
	}
	for (uint i = 0; i < nsym; i++) {
	    const char *sym = topsym[sym0[i]];
	    uint nf = 1 + n0dup[i];
	    for (uint j = 0; j < nf; j++) {
		uint fi = *fsym++;
		const char *fname = slab_get(slab, R->fname[fi]);
		uint32_t pkgref;
		memcpy(&pkgref, fname - 4, 4);
		const char *pkgname = slab_get(slab, pkgref);
		printf("%s.rpm\t%s\t%c\t%s\n", pkgname, fname, *T++, sym);
	    }
	}
    }
    return fsym;
}


void print_sym1(struct PkgRec *R, struct slab *slab, uint16_t *fsym, char *T)
{
    char s0[8192];
    size_t len0 = 0, lcp = 0;
    uint16_t *n0dup = R->n0dup + R->h.nsym0[0] + R->h.nsym0[1];
    char *s = R->sym1frenc;
    uint nsym = R->h.nsym1;
    for (uint i = 0; i < nsym; i++) {
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
	uint nf = 1 + n0dup[i];
	for (uint j = 0; j < nf; j++) {
	    uint fi = *fsym++;
	    const char *fname = slab_get(slab, R->fname[fi]);
	    uint32_t pkgref;
	    memcpy(&pkgref, fname - 4, 4);
	    const char *pkgname = slab_get(slab, pkgref);
	    printf("%s.rpm\t%s\t%c\t%s\n", pkgname, fname, *T++, s0);
	}
    }
}

int main(int argc, char **argv)
{
    struct slab slab;
    slab_init(&slab);
    char **topsym = load_topsym();
    for (int i = 1; i < argc; i++) {
	FILE *fp = fopen(argv[i], "r");
	assert(fp);
	uint32_t pkgname;
	do {
	    char *s = strrchr(argv[i], '/');
	    s = s ? s + 1 : argv[i];
	    char *dot = strrchr(s, '.');
	    assert(dot);
	    *dot = '\0';
	    pkgname = slab_put(&slab, s, dot - s + 1);
	} while (0);
	uint32_t reclen;
	do {
	    struct stat st;
	    int rc = fstat(fileno(fp), &st);
	    assert(rc == 0);
	    assert(st.st_size > sizeof(struct PkgRecHdr));
	    reclen = st.st_size - sizeof(struct PkgRecHdr);
	} while (0);
	struct PkgRec *R = PkgRec_read(fp, &slab, pkgname, reclen);
	assert(R);
	uint16_t *fsym =
	print_sym0(R, &slab, topsym);
	print_sym1(R, &slab, fsym, R->T + (fsym - R->fsym));
    }
    free(topsym);
    return 0;
}
