#!/bin/sh
#Copyright (C) 2026 Ivan Gaydardzhiev
#Licensed under the GPL-3.0-only

G='\033[0;32m'
R='\033[0;31m'
N='\033[0m'

[ ! -f ffvm_stage8 ] && make
[ ! -f assembler.ffvm ] && { printf "assembler.ffvm not found\n"; exit 1; }

fprint() {
	printf "[%s] Test: %-25s Result: %b\n" "$(date '+%Y-%m-%d %H:%M:%S')" "${1}" "${2}"
}

fh1() {
	tsrc=$(mktemp)
	tref=$(mktemp)
	tout=$(mktemp)
	printf 'p300jA:Aeh' > "${tsrc}"
	./ffvm_stage8 -r "${tsrc}" "${tref}" > /dev/null 2>&1
	./ffvm_stage8 assembler.ffvm "${tsrc}" "${tout}" > /dev/null 2>&1
	cmp -s "${tref}" "${tout}"
	rc="${?}"
	rm -f "${tsrc}" "${tref}" "${tout}"
	[ "${rc}" -eq 0 ] && {
		fprint "Selfhost Wide Operand" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Selfhost Wide Operand" "${R}FAILED${N}"
		return 1
	}
}

fh2() {
	tref=$(mktemp)
	tout=$(mktemp)
	./ffvm_stage8 -r assembler.ffvm "${tref}" > /dev/null 2>&1
	./ffvm_stage8 assembler.ffvm assembler.ffvm "${tout}"
	cmp -s "${tref}" "${tout}"
	rc="${?}"
	rm -f "${tref}" "${tout}"
	[ "${rc}" -eq 0 ] && {
		fprint "Selfhost Own Source" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Selfhost Own Source" "${R}FAILED${N}"
		return 2
	}
}

{ fh1 && fh2; ret="${?}"; } || exit 1

[ "${ret}" -eq 0 ] 2>/dev/null || printf "%s\n" "${ret}"
