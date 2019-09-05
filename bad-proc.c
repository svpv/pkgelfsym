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

// bad-proc - process ELF symbols as produced by rpmelfsym.pl and print the
// list of bad_elf_symbols (unresolved undefined symbols, with reference)
//
// This is a study of an efficient way of joining two lists / finding unmatched
// items.  We assume that the data is ready to be processed (i.e. how to store
// and fetch the data efficiently is another problem).

#include <stdio.h>
#include <stdbool.h>
#include <t1ha.h>
#include <fp47map.h>
#include "slab.h"

struct symline {
    char *line;
    size_t alloc;
    char *sym;
    size_t symlen;
    bool undefined;
    uint64_t lo, hi;
};

static bool getsymline(struct symline *S)
{
    // Some symbols aren't interesting, hence the loop.
    ssize_t len;
    while ((len = getline(&S->line, &S->alloc, stdin)) >= 0) {
	assert(len > 0);
	char *end = &S->line[--len];
	assert(*end == '\n');
	*end = '\t';
	char *tab1 = rawmemchr(S->line, '\t');
	assert(tab1 > S->line && tab1 < end);
	char *tab2 = rawmemchr(&tab1[1], '\t');
	assert(tab2 > &tab1[1] && tab2 < end);
	*end = *tab2 = '\0';
	char type = tab2[1];
	assert(tab2[2] == '\t');
	S->sym = &tab2[3];
	S->symlen = S->line + len - S->sym;
	switch (type) {
	case 'U':
	case 'T':
	case 'W':
	case 'V':
	case 'D':
	case 'B':
	case 'A':
	case 'R':
	case 'u':
	case 'i':
	case 'G':
	case 'S':
	    S->undefined = (type == 'U');
	    S->lo = t1ha2_atonce128(&S->hi, S->sym, S->symlen, 0);
	    return true;
	}
	assert((type >= 'A' && type <= 'Z') ||
	       (type >= 'a' && type <= 'z'));
    }
    return false;
}

// The hash table maps symbol names to these records.
struct rec {
    union {
	uint64_t hi;
	struct {
	    uint64_t flags: 8;
	    uint64_t hi56: 56; // extra hash to recheck the symbol name
	};
    };
    union {
	struct ext *ext; // external VLA, malloc'd
	struct {
	    uint32_t sym; // symbol name in the slab
	    uint32_t ref; // undefined symbols reference pkg+file
	};
    };
};

#define RF_EXT   01
#define RF_UNDEF 02

struct ext {
    uint32_t sym;
    uint32_t n;
    uint32_t ref[];
};

struct rec recs[1<<23];
unsigned nrec;

static struct slab slab;
static struct fp47map *map;

uint32_t doref(struct symline *S)
{
    static uint32_t lastref, lastlen;
    size_t len = S->sym - S->line - 3;
    if (len == lastlen) {
	const char *sym = slab_get(&slab, lastref);
	if (memcmp(S->line, sym, len) == 0)
	    return lastref;
    }
    lastref = slab_put(&slab, S->line, len + 1);
    lastlen = len;
    return lastref;
}

static void dosym(struct symline *S)
{
    struct rec *R;
    uint32_t mpos[FP47MAP_MAXFIND];
    unsigned n = fp47map_find(map, S->lo, mpos);
    for (unsigned i = 0; i < n; i++) {
	R = &recs[mpos[i]];
	if (R->hi >> 8 == S->hi >> 8)
	    goto found;
    }
    R = &recs[nrec];
    int rc = fp47map_insert(map, S->lo, nrec++);
    assert(rc >= 0);
    R->hi = S->hi >> 8 << 8;
    if (S->undefined) {
	R->hi |= RF_UNDEF;
	R->sym = slab_put(&slab, S->sym, S->symlen + 1);
	R->ref = doref(S);
    }
    return;
found:;
    struct ext *E;
    if (S->undefined) {
	if (R->flags & RF_EXT) {
	    E = R->ext;
	    // Extentding an extrnal array.
	    if ((E->n & (E->n - 1)) == 0) {
		E = R->ext = realloc(E, sizeof(*E) + 2 * E->n * sizeof(*E->ref));
		assert(E);
	    }
	    E->ref[E->n++] = doref(S);
	}
	else if (R->flags & RF_UNDEF) {
	    // Transforming one undef into an array.
	    uint32_t sym = R->sym, ref0 = R->ref;
	    E = R->ext = malloc(sizeof(*E) + 2 * sizeof(*E->ref));
	    assert(E);
	    R->flags = RF_EXT;
	    E->sym = sym;
	    E->n = 2;
	    E->ref[0] = ref0;
	    E->ref[1] = doref(S);

	}
	// Otherwise S is satisfied by R.
    }
    else {
	if (R->flags & RF_EXT) {
	    // Shutting down the array.
	    uint32_t sym = R->ext->sym;
	    free(R->ext);
	    R->sym = sym;
	}
	// We are completely satisfied.
	R->flags = 0, R->ref = 0;
    }
}

int main()
{
    slab_init(&slab);
    map = fp47map_new(22);
    struct symline *S1 = &(struct symline){ NULL };
    struct symline *S2 = &(struct symline){ NULL };
    bool ok = getsymline(S1);
    while (ok) {
	ok = getsymline(S2);
	fp47map_prefetch(map, S2->lo);
	dosym(S1);
	struct symline *tmp = S1;
	S1 = S2, S2 = tmp;
    }
    size_t iter = 0;
    uint32_t *ppos;
    while ((ppos = fp47map_next(map, &iter))) {
	struct rec *R = &recs[*ppos];
	if (R->flags & RF_EXT) {
	    struct ext *E = R->ext;
	    const char *sym = slab_get(&slab, E->sym);
	    for (uint32_t i = 0; i < E->n; i++) {
		const char *ref = slab_get(&slab, E->ref[i]);
		printf("%s\t%s\n", ref, sym);
	    }
	}
	else if (R->flags & RF_UNDEF) {
	    const char *sym = slab_get(&slab, R->sym);
	    const char *ref = slab_get(&slab, R->ref);
	    printf("%s\t%s\n", ref, sym);
	}
    }
    fp47map_free(map), map = NULL;
    return 0;
}
