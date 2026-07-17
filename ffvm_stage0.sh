#!/bin/sh

set -eu

N=${1:-20}

EXPR='if(eq(n\,0)\,0*(st(0\,1)+st(1\,1))+1\,ld(0)+0*(st(2\,ld(0)+ld(1))+st(0\,ld(1))+st(1\,ld(2))))'

ffmpeg -hide_banner -loglevel error \
	-f lavfi \
	-i "aevalsrc=${EXPR}:s=1:n=${N}:d=${N}" \
	-f f64le - \
	| od -t f8 -An \
	| tr ' ' '\n' \
	| grep -v '^$' \
	| awk '{printf "F(%d) = %.0f\n", NR-1, $1}'
