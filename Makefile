#Copyright (C) 2026 Ivan Gaydardzhiev
#Licensed under the GPL-3.0-only

CC:=$(shell command -v gcc 2>/dev/null || command -v musl-gcc 2>/dev/null || command -v tcc 2>/dev/null || command -v clang 2>/dev/null)
FLAGS=-static
BINS=ffvm_stage1 ffvm_stage2 ffvm_stage3 ffvm_stage4 ffvm_stage5 ffvm_stage6
ifeq ($(strip $(CC)),)
CC=cc
endif

all: $(BINS)

$(BINS): %: %.c
	$(CC) -o $@ $< $(FLAGS)

clean:
	rm $(BINS)
