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

struct SymRec {
    union {
	uint64_t hi;
	struct {
	    uint64_t flags: 8;
	    uint64_t hi56: 56; // extra hash to recheck the symbol name
	};
    };
    union {
	struct Ext *ext; // external VLA, malloc'd
	struct {
	    uint32_t sym; // symbol name in the slab
	    uint32_t ref; // undefined symbols reference pkg+file
	};
    };
};

#define SF_EXT   01
#define SF_DEF   02
#define SF_UNDEF 04

struct Ext {
    uint32_t sym;
    uint32_t n;
    uint32_t ref[];
};

struct Bad {
    struct slab slab;
    struct SymRec sym0rec[128<<10];
};

void Bad_init(struct Bad *B)
{
    slab_init(&B->slab);
    memset(&B->sym0rec, 0, sizeof B->sym0rec);
}

static void Bad_resolve0(struct SymRec *S, uint32_t fname, char T)
{
    struct Ext *E;
    if (T == 'U') {
	if (S->flags & SF_EXT) {
	    E = S->ext;
	    // Extentding an extrnal array.
	    if ((E->n & (E->n - 1)) == 0)
		E = S->ext = xrealloc(E, sizeof(*E) + 2 * E->n * sizeof(*E->ref));
	    E->ref[E->n++] = fname;
	}
	else if (S->flags & SF_UNDEF) {
	    // Transforming single undef into an array.
	    uint32_t ref0 = S->ref;
	    E = S->ext = xmalloc(sizeof(*E) + 2 * sizeof(*E->ref));
	    S->flags = SF_EXT;
	    E->n = 2;
	    E->ref[0] = ref0;
	    E->ref[1] = fname;
	}
	else if (S->flags & SF_DEF) {
	    // The undefined symbol is satisfied by S.
	}
	else {
	    // First undefined symbol.
	    S->flags = SF_UNDEF, S->ref = fname;
	}
    }
    else {
	if (S->flags & SF_EXT) {
	    // Shutting down the array.
	    free(S->ext);
	}
	// We are completely satisfied.
	S->flags = SF_DEF, S->ref = 0;
    }
}

uint16_t *Bad_proc0(struct Bad *B, struct PkgRec *R)
{
    uint16_t *sym0 = R->sym0;
    uint16_t *n0dup = R->n0dup;
    uint16_t *fsym = R->fsym;
    struct SymRec *sym0rec = B->sym0rec;
    char *T = R->T;
    uint nsym = R->h.nsym0[0];
    for (uint k = 0; k < 2; k++) {
	if (k) {
	    sym0 += nsym, n0dup += nsym;
	    nsym = R->h.nsym0[1];
	    sym0rec += 64<<10;
	}
	for (uint i = 0; i < nsym; i++) {
	    struct SymRec *S = &sym0rec[sym0[i]];
	    uint nf = 1 + n0dup[i];
	    for (uint j = 0; j < nf; j++) {
		uint fi = *fsym++;
		uint32_t fname = R->fname[fi];
		Bad_resolve0(S, fname, *T++);
	    }
	}
    }
    return fsym;
}

void Bad_proc1(struct Bad *B, struct PkgRec *R, uint16_t *fsym, char *T)
{
}

void Bad_print0(struct Bad *B)
{
    char **topsym = load_topsym();
    for (uint i = 0; i < (128<<10); i++) {
	struct SymRec *S = &B->sym0rec[i];
	const char *sym = topsym[i];
	if (S->flags & SF_EXT) {
	    struct Ext *E = S->ext;
	    for (uint32_t i = 0; i < E->n; i++) {
		const char *fname = slab_get(&B->slab, E->ref[i]);
		uint32_t pkgref;
		memcpy(&pkgref, fname - 4, 4);
		const char *pkgname = slab_get(&B->slab, pkgref);
		printf("%s.rpm\t%s\t%s\n", pkgname, fname, sym);
	    }
	}
	else if (S->flags & SF_UNDEF) {
	    const char *fname = slab_get(&B->slab, S->ref);
	    uint32_t pkgref;
	    memcpy(&pkgref, fname - 4, 4);
	    const char *pkgname = slab_get(&B->slab, pkgref);
	    printf("%s.rpm\t%s\t%s\n", pkgname, fname, sym);
	}
    }
    free(topsym);
}

void Bad_print1(struct Bad *B)
{
}

int main(int argc, char **argv)
{
    struct Bad B;
    Bad_init(&B);
    for (int i = 1; i < argc; i++) {
	FILE *fp = fopen(argv[i], "r");
	assert(fp);
	uint32_t pkgname;
	{
	    char *s = strrchr(argv[i], '/');
	    s = s ? s + 1 : argv[i];
	    char *dot = strrchr(s, '.');
	    assert(dot);
	    *dot = '\0';
	    pkgname = slab_put(&B.slab, s, dot - s + 1);
	}
	uint32_t reclen;
	{
	    struct stat st;
	    int rc = fstat(fileno(fp), &st);
	    assert(rc == 0);
	    if (st.st_size == 0)
		continue;
	    assert(st.st_size > sizeof(struct PkgRecHdr));
	    reclen = st.st_size - sizeof(struct PkgRecHdr);
	}
	struct PkgRec *R = PkgRec_read(fp, &B.slab, pkgname, reclen);
	assert(R);
	uint16_t *fsym =
	Bad_proc0(&B, R);
	Bad_proc1(&B, R, fsym, R->T + (fsym - R->fsym));
    }
    Bad_print0(&B);
    Bad_print1(&B);
    return 0;
}
