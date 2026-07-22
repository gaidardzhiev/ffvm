#!/bin/sh
#Copyright (C) 2026 Ivan Gaydardzhiev
#Licensed under the GPL-3.0-only

G='\033[0;32m'
R='\033[0;31m'
N='\033[0m'

[ ! -f ffvm_stage0.sh ] && { printf "ffvm_stage0.sh not found\n"; exit 1; }
[ ! -f ffvm_stage1 ] && make
[ ! -f ffvm_stage2 ] && make
[ ! -f ffvm_stage3 ] && make
[ ! -f ffvm_stage4 ] && make
[ ! -f ffvm_stage5 ] && make
[ ! -f ffvm_stage6 ] && make
[ ! -f ffvm_stage7 ] && make

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

ft2a() {
	captured=$(./ffvm_stage2 "push:3,push:4,add,emit")
	expected="result: 7"
	[ "${captured}" = "${expected}" ] && {
		fprint "Stage2 Add" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Stage2 Add" "${R}FAILED${N}"
		printf "expected:\n%s\ncaptured:\n%s\n\n" "${expected}" "${captured}"
		return 4
	}
}

ft2b() {
	captured=$(./ffvm_stage2 "push:10,push:3,sub,emit")
	expected="result: 7"
	[ "${captured}" = "${expected}" ] && {
		fprint "Stage2 Sub" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Stage2 Sub" "${R}FAILED${N}"
		printf "expected:\n%s\ncaptured:\n%s\n\n" "${expected}" "${captured}"
		return 5
	}
}

ft2c() {
	captured=$(./ffvm_stage2 "push:6,push:7,mul,emit")
	expected="result: 42"
	[ "${captured}" = "${expected}" ] && {
		fprint "Stage2 Mul" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Stage2 Mul" "${R}FAILED${N}"
		printf "expected:\n%s\ncaptured:\n%s\n\n" "${expected}" "${captured}"
		return 6
	}
}

ft2d() {
	captured=$(./ffvm_stage2 "push:20,push:4,div,emit")
	expected="result: 5"
	[ "${captured}" = "${expected}" ] && {
		fprint "Stage2 Div" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Stage2 Div" "${R}FAILED${N}"
		printf "expected:\n%s\ncaptured:\n%s\n\n" "${expected}" "${captured}"
		return 7
	}
}

ft2e() {
	captured=$(./ffvm_stage2 "push:5,dup,mul,emit")
	expected="result: 25"
	[ "${captured}" = "${expected}" ] && {
		fprint "Stage2 Dup" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Stage2 Dup" "${R}FAILED${N}"
		printf "expected:\n%s\ncaptured:\n%s\n\n" "${expected}" "${captured}"
		return 8
	}
}

ft2f() {
	captured=$(./ffvm_stage2 "push:2,push:3,push:4,mul,add,emit")
	expected="result: 14"
	[ "${captured}" = "${expected}" ] && {
		fprint "Stage2 Compound" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Stage2 Compound" "${R}FAILED${N}"
		printf "expected:\n%s\ncaptured:\n%s\n\n" "${expected}" "${captured}"
		return 9
	}
}

ft2g() {
	captured=$(./ffvm_stage2 "push:1,push:2,swap,sub,emit")
	expected="result: 1"
	[ "${captured}" = "${expected}" ] && {
		fprint "Stage2 Swap" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Stage2 Swap" "${R}FAILED${N}"
		printf "expected:\n%s\ncaptured:\n%s\n\n" "${expected}" "${captured}"
		return 10
	}
}

ft3a() {
	captured=$(./ffvm_stage3 "push:3,push:4,add,emit,halt")
	expected="result: 7"
	[ "${captured}" = "${expected}" ] && {
		fprint "Stage3 Fetch" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Stage3 Fetch" "${R}FAILED${N}"
		printf "expected:\n%s\ncaptured:\n%s\n\n" "${expected}" "${captured}"
		return 11
	}
}

ft3b() {
	captured=$(./ffvm_stage3 "push:1,jmp:skip,push:99,skip:,emit,halt")
	expected="result: 1"
	[ "${captured}" = "${expected}" ] && {
		fprint "Stage3 Jump" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Stage3 Jump" "${R}FAILED${N}"
		printf "expected:\n%s\ncaptured:\n%s\n\n" "${expected}" "${captured}"
		return 12
	}
}

ft3c() {
	captured=$(./ffvm_stage3 "push:2,push:3,gt,jz:l1,push:111,jmp:l2,l1:,push:222,l2:,emit,halt")
	expected="result: 222"
	[ "${captured}" = "${expected}" ] && {
		fprint "Stage3 Branch" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Stage3 Branch" "${R}FAILED${N}"
		printf "expected:\n%s\ncaptured:\n%s\n\n" "${expected}" "${captured}"
		return 13
	}
}

ft3d() {
	captured=$(./ffvm_stage3 "push:5,loop:,push:1,sub,dup,jnz:loop,emit,halt")
	expected="result: 0"
	[ "${captured}" = "${expected}" ] && {
		fprint "Stage3 Loop" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Stage3 Loop" "${R}FAILED${N}"
		printf "expected:\n%s\ncaptured:\n%s\n\n" "${expected}" "${captured}"
		return 14
	}
}

ft3e() {
	captured=$(./ffvm_stage3 "push:0,storer:0,push:5,loop:,dup,loadr:0,add,storer:0,push:1,sub,dup,jnz:loop,pop,loadr:0,emit,halt")
	expected="result: 15"
	[ "${captured}" = "${expected}" ] && {
		fprint "Stage3 Accumulator" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Stage3 Accumulator" "${R}FAILED${N}"
		printf "expected:\n%s\ncaptured:\n%s\n\n" "${expected}" "${captured}"
		return 15
	}
}

ft4a() {
	captured=$(./ffvm_stage4 "push:42,store:7,load:7,emit,halt")
	expected="result: 42"
	[ "${captured}" = "${expected}" ] && {
		fprint "Stage4 Store" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Stage4 Store" "${R}FAILED${N}"
		printf "expected:\n%s\ncaptured:\n%s\n\n" "${expected}" "${captured}"
		return 16
	}
}

ft4b() {
	captured=$(./ffvm_stage4 "push:99,push:5,storex,load:5,emit,halt")
	expected="result: 99"
	[ "${captured}" = "${expected}" ] && {
		fprint "Stage4 Storex" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Stage4 Storex" "${R}FAILED${N}"
		printf "expected:\n%s\ncaptured:\n%s\n\n" "${expected}" "${captured}"
		return 17
	}
}

ft4c() {
	captured=$(./ffvm_stage4 "push:77,store:9,push:9,loadx,emit,halt")
	expected="result: 77"
	[ "${captured}" = "${expected}" ] && {
		fprint "Stage4 Loadx" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Stage4 Loadx" "${R}FAILED${N}"
		printf "expected:\n%s\ncaptured:\n%s\n\n" "${expected}" "${captured}"
		return 18
	}
}

ft4d() {
	captured=$(./ffvm_stage4 "push:1,store:3,push:2,store:3,load:3,emit,halt")
	expected="result: 2"
	[ "${captured}" = "${expected}" ] && {
		fprint "Stage4 Overwrite" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Stage4 Overwrite" "${R}FAILED${N}"
		printf "expected:\n%s\ncaptured:\n%s\n\n" "${expected}" "${captured}"
		return 19
	}
}

ft4e() {
	captured=$(./ffvm_stage4 "push:0,storer:0,w:,loadr:0,push:1,add,loadr:0,storex,loadr:0,push:1,add,storer:0,loadr:0,push:3,lt,jnz:w,push:0,loadx,push:1,loadx,add,push:2,loadx,add,emit,halt")
	expected="result: 6"
	[ "${captured}" = "${expected}" ] && {
		fprint "Stage4 Memory Loop" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Stage4 Memory Loop" "${R}FAILED${N}"
		printf "expected:\n%s\ncaptured:\n%s\n\n" "${expected}" "${captured}"
		return 20
	}
}

ft5a() {
	captured=$(./ffvm_stage5 "push:9,probe,halt")
	expected="result: 9"
	[ "${captured}" = "${expected}" ] && {
		fprint "Stage5 Probe Peek" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Stage5 Probe Peek" "${R}FAILED${N}"
		printf "expected:\n%s\ncaptured:\n%s\n\n" "${expected}" "${captured}"
		return 21
	}
}

ft5b() {
	captured=$(./ffvm_stage5 "push:3,probe,push:4,add,emit,halt")
	expected="result: 7"
	[ "${captured}" = "${expected}" ] && {
		fprint "Stage5 Probe Nondestruct" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Stage5 Probe Nondestruct" "${R}FAILED${N}"
		printf "expected:\n%s\ncaptured:\n%s\n\n" "${expected}" "${captured}"
		return 22
	}
}

ft5c() {
	captured=$(./ffvm_stage5 "push:0,storer:0,w:,loadr:0,push:1,add,storer:0,loadr:0,probe,loadr:0,push:3,lt,jnz:w,loadr:0,emit,halt")
	expected="result: 3"
	[ "${captured}" = "${expected}" ] && {
		fprint "Stage5 Probe In Loop" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Stage5 Probe In Loop" "${R}FAILED${N}"
		printf "expected:\n%s\ncaptured:\n%s\n\n" "${expected}" "${captured}"
		return 23
	}
}

ft6a() {
	captured=$(./ffvm_stage6 "push:300,push:500,add,emit,halt")
	expected="result: 800"
	[ "${captured}" = "${expected}" ] && {
		fprint "Stage6 Wide Add" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Stage6 Wide Add" "${R}FAILED${N}"
		printf "expected:\n%s\ncaptured:\n%s\n\n" "${expected}" "${captured}"
		return 24
	}
}

ft6b() {
	captured=$(./ffvm_stage6 "push:40000,store:15000,load:15000,emit,halt")
	expected="result: 40000"
	[ "${captured}" = "${expected}" ] && {
		fprint "Stage6 High Memory" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Stage6 High Memory" "${R}FAILED${N}"
		printf "expected:\n%s\ncaptured:\n%s\n\n" "${expected}" "${captured}"
		return 25
	}
}

ft6c() {
	captured=$(./ffvm_stage6 "push:321,push:15800,storex,push:15800,loadx,emit,halt")
	expected="result: 321"
	[ "${captured}" = "${expected}" ] && {
		fprint "Stage6 High Indirect" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Stage6 High Indirect" "${R}FAILED${N}"
		printf "expected:\n%s\ncaptured:\n%s\n\n" "${expected}" "${captured}"
		return 26
	}
}

ft6d() {
	tin=$(mktemp)
	tout=$(mktemp)
	printf 'Hi' > "${tin}"
	./ffvm_stage6 "push:0,storer:0,loop:,loadr:0,load:15809,lt,jz:done,loadr:0,loadx,push:7650,loadr:0,add,storex,loadr:0,push:1,add,storer:0,jmp:loop,done:,load:15809,store:15808,halt" "${tin}" "${tout}" >/dev/null 2>&1
	captured=$(cat "${tout}")
	expected="Hi"
	rm -f "${tin}" "${tout}"
	[ "${captured}" = "${expected}" ] && {
		fprint "Stage6 Tape Echo" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Stage6 Tape Echo" "${R}FAILED${N}"
		printf "expected:\n%s\ncaptured:\n%s\n\n" "${expected}" "${captured}"
		return 27
	}
}

ft7a() {
	captured=$(./ffvm_stage7 "p300p500+eh")
	expected="result: 800"
	[ "${captured}" = "${expected}" ] && {
		fprint "Stage7 Compact Add" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Stage7 Compact Add" "${R}FAILED${N}"
		printf "expected:\n%s\ncaptured:\n%s\n\n" "${expected}" "${captured}"
		return 28
	}
}

ft7b() {
	captured=$(./ffvm_stage7 "p40000t5000l5000eh")
	expected="result: 40000"
	[ "${captured}" = "${expected}" ] && {
		fprint "Stage7 Compact Memory" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Stage7 Compact Memory" "${R}FAILED${N}"
		printf "expected:\n%s\ncaptured:\n%s\n\n" "${expected}" "${captured}"
		return 29
	}
}

ft7c() {
	captured=$(./ffvm_stage7 "p7jEp99:Eeh")
	expected="result: 7"
	[ "${captured}" = "${expected}" ] && {
		fprint "Stage7 Compact Label" "${G}PASSED${N}"
		return 0
	} || {
		fprint "Stage7 Compact Label" "${R}FAILED${N}"
		printf "expected:\n%s\ncaptured:\n%s\n\n" "${expected}" "${captured}"
		return 30
	}
}

{ ft0 && ft1 && ft2a && ft2b && ft2c && ft2d && ft2e && ft2f && ft2g && ft3a && ft3b && ft3c && ft3d && ft3e && ft4a && ft4b && ft4c && ft4d && ft4e && ft5a && ft5b && ft5c && ft6a && ft6b && ft6c && ft6d && ft7a && ft7b && ft7c; ret="${?}"; } || exit 1

[ "${ret}" -eq 0 ] 2>/dev/null || printf "%s\n" "${ret}"
