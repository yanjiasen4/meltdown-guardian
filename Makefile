# Meltdown-guardian Makefile
# Author: Ming.yang@intel.com

OBJ := meltdown-guardian.c
OUT := meltdown-guardian
CC ?= gcc

CFLAGS := -g -O2 -std=c++14

.PHONY: all
all:
	$(CC) $(OBJ) -o $(OUT) $(CFLAGS)

.PHONY: clean
clean:
	rm -rf $(OUT)
