# Makefile
#
# Copyright (c) 2006   Regents of the University of California
#

ATSC_DIR = ..
DISTRIB_DIR = /home/path

INCLUDE_DIR = $(ATSC_DIR)/include
DB_DIR = $(DISTRIB_DIR)/db
LOCAL_DIR = $(DISTRIB_DIR)/local

CC = cc
CFLAGS += -c -D_GNU_SOURCE
CFLAGS += -I. -I$(INCLUDE_DIR) -I$(DB_DIR) -I$(LOCAL_DIR) 
CFLAGS += -g
			
AR = ar
AR_FLAGS += rv

LINK = cc
LFLAGS += -L$(LOCAL_DIR)  -L$(DB_DIR)  

LIBS= -ldb_clts -llocals  -lrt

EXEC = rd3418 rd34182 

.c.o:	
	$(CC) $(CFLAGS) $*.c -o $*.o 
.o:
	$(LINK) $(LFLAGS) -o $@ $@.o $(LIBS) 

all: $(EXEC)

rd3418:		rd3418.o 

rd34182.o:	rd3418.c 
	$(CC) $(CFLAGS) -DCOMPARE -o rd34182.o rd3418.c
rd34182:	rd34182.o 
	$(LINK) $(LFLAGS) -o $@ rd34182.o $(LIBS)

clean:
	rm -f *.o *.err *.a $(EXEC)

install:
	cp $(EXEC) $(ATSC_DIR)/test
