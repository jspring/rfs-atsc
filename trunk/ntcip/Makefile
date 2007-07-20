# Makefile
#
# Copyright (c) 2006   Regents of the University of California
#

ATSC_DIR = ..
DISTRIB_DIR = ../../path

INCLUDE_DIR = $(ATSC_DIR)/include
DB_DIR = $(DISTRIB_DIR)/db
LOCAL_DIR = $(DISTRIB_DIR)/local

CC = cc
CFLAGS += -c -D_GNU_SOURCE
CFLAGS += -I. -I$(INCLUDE_DIR) -I$(DB_DIR) -I$(LOCAL_DIR)
CFLAGS += -g

LINK = cc 
LFLAGS += -L$(DB_DIR) -L$(LOCAL_DIR) -L. 

LIBS = -ldb_clts -llocals -lrt 

EXEC = rdntcip rdntcip2

HDRS = atsc.h mibdefs.h 

.c.o:	$(HDRS) 
	$(CC) $(CFLAGS) $*.c -o $*.o

.o:	
	$(LINK) $(LFLAGS) -o $@ $@.o $(LIBS)

all: 	$(EXEC)

asc2_get.o: asc_get.c 
	$(CC) $(CFLAGS) -DCOMPARE -o asc2_get.o asc_get.c
parse_asc2.o: parse_asc.c 
	$(CC) $(CFLAGS) -DCOMPARE -o parse_asc2.o parse_asc.c

rdntcip: asc_get.o parse_asc.o format_asc.o fcs.o
	$(LINK) $(LFLAGS) -o $@ asc_get.o parse_asc.o format_asc.o fcs.o $(LIBS)
rdntcip2: asc2_get.o parse_asc2.o format_asc.o fcs.o
	$(LINK) $(LFLAGS) -o $@ asc2_get.o parse_asc2.o format_asc.o fcs.o $(LIBS)

clean:
	\rm -f *.o *.*~ $(EXEC)

install:
	cp $(EXEC) $(ATSC_DIR)/test
