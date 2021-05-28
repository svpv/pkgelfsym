#include <stdio.h>
#include "platform.h"

struct PkgRecHdr {
    uint16_t flags; // compression flags
    uint16_t n0files; // number of files - 1
    uint16_t nsym0[2]; // topsym.list counts
    uint32_t nsym1; // frenc-mini symbol count
    uint32_t nsym; // total symbol count (with dups from all files)
    uint32_t frenclen; // uncompressed len of frenc-mini segment (malloc hint)
};

struct PkgRec {
    struct PkgRecHdr h;
    uint16_t *sym0; // decoded sym0 integers
    uint16_t *fsym; // file reference for each symbol
    uint16_t *n0dup; // dup count for each unique symbol - 1
    char *sym1frenc; // frenc-mini blob, LZ-decompressed
    uint32_t sym1frenclen;
    uint32_t fname[]; // maps file references into slab
};

struct slab;

struct PkgRec *PkgRec_read(FILE *fp, struct slab *slab, uint32_t pkgname, uint32_t reclen);
