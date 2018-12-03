# Meltdown-guardian Makefile
# Author: Ming.yang@intel.com

SRC := meltdown-guardian.c msrop.c
DEP := meltdown-guardian.h msrop.h
PROG := mg
OBJ := msrop.o meltdown-guardian.o
OUT := meltdown-guardian
CC ?= gcc

CFLAGS := -g -O2

.PHONY: all
all: $(OBJ)
	$(CC) -g -o $(OUT) $(OBJ)

msrop.o: msrop.c msrop.h
	$(CC) -g -c msrop.c

meltdown-guardian.o: meltdown-guardian.c meltdown-guardian.h
	$(CC) -g -c meltdown-guardian.c

.PHONY: clean
clean:
	rm -rf $(OBJ) $(OUT)
