# Makefile
#
# Copyright (c) 2006   Regents of the University of California
#

ATSC_DIR = ..
DISTRIB_DIR = ../../path

IX_DIR = $(ATSC_DIR)/ix
INCLUDE_DIR = $(ATSC_DIR)/include
DB_DIR = $(DISTRIB_DIR)/db
LOCAL_DIR = $(DISTRIB_DIR)/local

CC = cc
CFLAGS += -c -D_GNU_SOURCE
CFLAGS += -I. -I$(INCLUDE_DIR) -I$(DB_DIR) -I$(LOCAL_DIR) -I$(IX_DIR)
CFLAGS += -g
			
AR = ar
AR_FLAGS += rv

LINK = cc
LFLAGS += -L$(DB_DIR) -L$(LOCAL_DIR) -L$(IX_DIR)

LIBS = -lix -ldb_clts -llocals -lrt

EXEC = phases compare_ntcip compare_3418

.c.o:	
	$(CC) $(CFLAGS) $*.c -o $*.o 
.o:
	$(LINK) $(LFLAGS) -o $@ $@.o $(LIBS) 

all: $(EXEC)

phases:		$(OBJS) phases.o
compare_ntcip:	$(OBJS) compare_ntcip.o
compare_3418:	$(OBJS) compare_3418.o

clean:
	rm -f *.o *.err *.a $(EXEC)

install:
	cp $(EXEC) $(ATSC_DIR)/test
