#Stephen Bangs
#Lab 2 Makefile - read, write, archive
#4/29/25
#Creates a functional arvik archive tool to collate, read and write files on babbage using rchaney's arvik.h
#Also cleans, and tars files using make clean and make tar commands.
#added a git lazy command for source control, though I mostly still commit manually.

DEBUG = -g3 -O0

CFLAGS = -Wall -Wextra -Wshadow -Wunreachable-code -Wredundant-decls -Wmissing-declarations -Wold-style-definition -Wmissing-prototypes -Wdeclaration-after-statement -Wno-return-local-addr -Wunsafe-loop-optimizations -Wuninitialized -Werror

CC = gcc

arvik = arvik

TARGETS = $(arvik)
CSRCS = $(arvik).c
COBJS = $(arvik).o

all: ${TARGETS}

.PHONY: clean tar

$(COBJS): $(CSRCS)
	$(CC) $(CFLAGS) -c $(@:.o=.c) -lz

$(TARGETS): $(COBJS)
	$(CC) $(@).o -o $(@)

clean:
	rm -f $(COBJS) $(TARGETS) *~ *.txt *.diff *.ltoc *.stoc *.arv *.bin

LAB = 02
TAR_FILE = stbangs_Lab$(LAB).tar.gz

tar:
	rm -f $(TAR_FILE)
	tar cvfa $(TAR_FILE) ?akefile *.[ch]
	tar tvaf $(TAR_FILE)

git lazy:
	git add *.[ch] ?akefile
	git commit -m "lazy make git commit"
