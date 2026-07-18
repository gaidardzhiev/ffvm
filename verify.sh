#!/bin/sh
#Copyright (C) 2026 Ivan Gaydardzhiev
#Licensed under the GPL-3.0-only

G='\033[0;32m'
R='\033[0;31m'
N='\033[0m'

[ ! -f ffvm_stage0.sh ] && { printf "ffvm_stage0.sh not found\n"; exit 1; }
[ ! -f ffvm_stage1 ] && make

fprint() {
	printf "[%s] Test: %-25s Result: %b\n" "$(date '+%Y-%m-%d %H:%M:%S')" "${1}" "${2}"
}

ft0() {
	captured=$(sh ffvm_stage0.sh 10)
	expected="F(0) = 1
F(1) = 1
F(2) = 1
F(3) = 2
F(4) = 3
F(5) = 5
F(6) = 8
F(7) = 13
F(8) = 21
F(9) = 34"
	[ "${captured}" = "${expected}" ] && {
		fprint "Stage0 Fibonacci" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Stage0 Fibonacci" "${R}FAILED${N}"
		printf "expected:\n%s\ncaptured:\n%s\n\n" "${expected}" "${captured}"
		return 2
	}
}

ft1() {
	captured=$(./ffvm_stage1 2>&1 | tail -2)
	expected="65536 pixels checked, 0 errors
stage 1 complete"
	[ "${captured}" = "${expected}" ] && {
		fprint "Stage1 Pixel Memory" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Stage1 Pixel Memory" "${R}FAILED${N}"
		printf "expected:\n%s\ncaptured:\n%s\n\n" "${expected}" "${captured}"
		return 3
	}
}

{ ft0 && ft1; ret="${?}"; } || exit 1

[ "${ret}" -eq 0 ] 2>/dev/null || printf "%s\n" "${ret}"
