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
#include "slab.h"

struct symline {
    char *line;
    size_t alloc;
    char *sym;
    size_t symlen;
    bool undefined;
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
	    return true;
	}
	assert((type >= 'A' && type <= 'Z') ||
	       (type >= 'a' && type <= 'z'));
    }
    return false;
}

int main()
{
    struct symline S = { NULL, };
    size_t cnt = 0;
    while (getsymline(&S))
	cnt += S.symlen + 1;
    printf("%zu\n", cnt);
    return 0;
}
