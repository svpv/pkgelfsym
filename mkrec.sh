#!/bin/sh -efu

rm -rf .tmp
mkdir .tmp
T=.tmp/pkg
perl -lne 'printf"%05x\t%s\n",$.-1,$_' <${0%/*}/topsym.list >$T.topsym

for f do
    b=${f##*/}; b=${b%.rpm}
    echo >&2 "$b"

    rpmelfsym.pl "$f" |
    awk -F'\t' '
	BEGIN {
	    Junk["__bss_end__"]
	    Junk["__bss_start"]
	    Junk["__bss_start__"]
	    Junk["__data_start"]
	    Junk["__end__"]
	    Junk["__gmon_start__"]
	    Junk["__libc_csu_fini"]
	    Junk["__libc_csu_init"]
	    Junk["_bss_end__"]
	    Junk["_edata"]
	    Junk["_end"]
	    Junk["_fini"]
	    Junk["_init"]
	    Junk["_start"]
	    Junk["data_start"]
	    Junk["main"]
	}
	index("UTWVDBRuiGS", $3) &&
	!($4 in Junk) &&
	index($2, "/usr/share/doc/") != 1 {
	    print $2 "\t" $3 "\t" $4
	}
    ' |
    sort -t$'\t' -u -k3 -k1,1 >$T.f+T+sym
    if ! [ -s $T.f+T+sym ]; then
	echo >&2 "no symbols in $b"
	: >$b.rec
	continue
    fi

    cut -f1 $T.f+T+sym |sort -u |perl -lne 'printf"%04x\t%s\n",$.-1,$_' >$T.nf
    cut -f2 $T.nf |frenc-mini >$T.frenc

    join -t$'\t'     -12 -23 $T.topsym $T.f+T+sym -o '2.1 2.2 0' |sort >$T.f+T+sym0
    join -t$'\t' -v2 -12 -23 $T.topsym $T.f+T+sym -o '2.1 2.2 0' |sort >$T.f+T+sym1

    join -t$'\t' -12 -21 $T.nf $T.f+T+sym0 -o '1.1 2.2 2.3' |sort -t$'\t' -k3 >$T.nf+T+sym0
    join -t$'\t' -12 -21 $T.nf $T.f+T+sym1 -o '1.1 2.2 2.3' |sort -t$'\t' -k3 >$T.nf+T+sym1

    cut -f1 $T.nf+T+sym{0,1} >$T.fsym
    cut -f3 $T.nf+T+sym{0,1} |uniq -c |perl -lne 'printf"%04x\n",$_-1' >$T.n0dup
    join -t$'\t' -12 -23 -o 1.1 $T.topsym $T.nf+T+sym0 |uniq >$T.sym0i
    cut -f3 <$T.nf+T+sym1 |uniq |frenc-mini >>$T.frenc

    ${0%/*}/packrec $T.sym0i $T.fsym $T.n0dup $T.frenc >$b.rec
done
