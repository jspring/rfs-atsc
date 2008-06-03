# Makefile
#
# Copyright (c) 2006   Regents of the University of California
#

ATSC_DIR = ..
DISTRIB_DIR = /home/path
CICAS_DIR = /home/cicas
IX_DIR = $(ATSC_DIR)/ix
EXP_DIR = $(CICAS_DIR)/exp
INCLUDE_DIR = $(ATSC_DIR)/include
DB_DIR = $(DISTRIB_DIR)/db
LOCAL_DIR = $(DISTRIB_DIR)/local
CICAS_DIR = ../../cicas

CC = cc
CFLAGS += -c -D_GNU_SOURCE
CFLAGS += -I. -I$(INCLUDE_DIR) -I$(DB_DIR) -I$(LOCAL_DIR) -I$(IX_DIR) -I$(EXP_DIR)
CFLAGS += -I$(CICAS_DIR)/exp
CFLAGS += -g
			
AR = ar
AR_FLAGS += rv

LINK = cc
LFLAGS += -L$(DB_DIR) -L$(LOCAL_DIR) -L$(IX_DIR)

LIBS = -lix -ldb_clts -llocals -lrt

EXEC = sendatsc2tlab

.c.o:	
	$(CC) $(CFLAGS) $*.c -o $*.o 
.o:
	$(LINK) $(LFLAGS) -o $@ $@.o $(LIBS) 

all: $(EXEC)

sendatsc2tlab:	$(OBJS) sendatsc2tlab.o

clean:
	rm -f *.o *.err *.a $(EXEC)

install:
	cp $(EXEC) $(ATSC_DIR)/test
