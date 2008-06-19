# Makefile
#
# Copyright (c) 2006   Regents of the University of California
#

ATSC_DIR = ..
DISTRIB_DIR = /home/path
TSP_DIR = /home/tsp
INCLUDE_DIR = $(ATSC_DIR)/include
DB_DIR = $(DISTRIB_DIR)/db
LOCAL_DIR = $(DISTRIB_DIR)/local
SENS_INCLUDE = $(DISTRIB_DIR)/sens/include

ifndef PATH_CC
PATH_CC = gcc
endif

PATH_CFLAGS += -c -D_GNU_SOURCE -I$(TSP_DIR)/include
PATH_CFLAGS += -I. -I$(INCLUDE_DIR) -I$(DB_DIR) -I$(LOCAL_DIR) -I$(SENS_INCLUDE)
PATH_CFLAGS += -g
			
ifndef PATH_AR
AR = ar
endif

AR_FLAGS += rv

ifndef PATH_LINK
PATH_LINK = gcc
endif

PATH_LDFLAGS += -L$(DB_DIR) -L$(LOCAL_DIR) -L.

LIBS = -lix -ldb_clts -llocals  -lrt
			
HDRS = ix_msg.h

IX_OBJS = ix_utils.o ix_db_utils.o ix_file_utils.o

EXEC = rcv_ix_msg snd_ix_msg ix_byte_count

.c.o:	$(HDRS) 
	$(PATH_CC) $(CFLAGS) $(PATH_CFLAGS) $*.c -o $*.o 
.o:	libix.a
	$(PATH_LINK) $(PATH_LDFLAGS) -o $@ $@.o $(LIBS) 

all: libix.a $(EXEC)

libix.a: $(IX_OBJS)
	$(PATH_AR) $(AR_FLAGS) $@ $(IX_OBJS)

rcv_ix_msg:	rcv_ix_msg.o
snd_ix_msg:	snd_ix_msg.o
ix_byte_count:	ix_byte_count.o

clean:
	rm -f *.o *.err *.a $(EXEC)

install:
	cp $(EXEC) $(ATSC_DIR)/test
