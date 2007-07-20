# Makefile
#
# Copyright (c) 2006   Regents of the University of California
#

ATSC_DIR = ..
DISTRIB_DIR = ../..//path

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
LFLAGS += -L$(DB_DIR) -L$(LOCAL_DIR) -L.

LIBS = -lix -ldb_clts -llocals  -lrt
			
HDRS = ix_msg.h

IX_OBJS = ix_utils.o ix_db_utils.o ix_file_utils.o

EXEC = rcv_ix_msg snd_ix_msg ix_byte_count

.c.o:	$(HDRS) 
	$(CC) $(CFLAGS) $*.c -o $*.o 
.o:	libix.a
	$(LINK) $(LFLAGS) -o $@ $@.o $(LIBS) 

all: libix.a $(EXEC)

libix.a: $(IX_OBJS)
	$(AR) $(AR_FLAGS) $@ $(IX_OBJS)

rcv_ix_msg:	rcv_ix_msg.o
snd_ix_msg:	snd_ix_msg.o
ix_byte_count:	ix_byte_count.o

clean:
	rm -f *.o *.err *.a $(EXEC)

install:
	cp $(EXEC) $(ATSC_DIR)/test
