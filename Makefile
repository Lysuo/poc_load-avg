.PHONY: clean
.SUFFIXES:

CC = gcc
SRC ?= fork.c

CFLAGS ?= -Wextra -Wall
CFLAGS += -g

ifeq ($(WITH_THREADS),1)
CFLAGS += -DWITH_INNER_THREADS
LIBS ?= -lpthread
endif

VERB=@

all:
	$(VERB) $(CC) $(CFLAGS) $(LDFLAGS) $(SRC) $(LIBS)

