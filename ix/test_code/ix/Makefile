# Makefile for TEST ONLY
#
# Copyright (c) 2006   Regents of the University of California
#

INCLUDE_DIR = /home/atsc/include
LOCAL_DIR = /home/path/local

CC = cc
CFLAGS += -c -D_GNU_SOURCE -DTEST_ONLY
CFLAGS += -I. -I$(INCLUDE_DIR) -I$(LOCAL_DIR)
CFLAGS += -g
			
AR = ar
AR_FLAGS += rv

LINK = cc
LFLAGS += -L$(LOCAL_DIR) -L.

LIBS = -llocals -lrt

HDRS = ix_msg.h

OBJS = ix_utils.o ix_db_utils.o ix_file_utils.o

EXEC = test_rcv_ix_msg test_snd_ix_msg ix_byte_count

.c.o:	$(HDRS) 
	$(CC) $(CFLAGS) $*.c -o $*.o 

all: $(EXEC)

test_rcv_ix_msg: $(OBJS) rcv_ix_msg.o
	$(LINK) $(LFLAGS) -o $@ rcv_ix_msg.o $(OBJS) $(LIBS)

test_snd_ix_msg: $(OBJS) snd_ix_msg.o
	$(LINK) $(LFLAGS) -o $@ snd_ix_msg.o $(OBJS) $(LIBS)

ix_byte_count:	$(OBJS) ix_byte_count.o
	$(LINK) $(LFLAGS) -o $@ ix_byte_count.o $(OBJS) $(LIBS)

clean:
	rm -f *.o *.err *.a $(EXEC)

install:
	cp $(EXEC) ../test
