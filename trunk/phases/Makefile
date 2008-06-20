# Makefile
#
# Copyright (c) 2006   Regents of the University of California
#

ATSC_DIR = ..
ifndef PATH_AR
DISTRIB_DIR = ../../path 
else
DISTRIB_DIR = /home/gems/sdk/sobos/build_dir/i386/path_util-1.0
endif
CICAS_DIR = /home/cicas
IX_DIR = $(ATSC_DIR)/ix
EXP_DIR = $(CICAS_DIR)/exp
INCLUDE_DIR = $(ATSC_DIR)/include
DB_DIR = $(DISTRIB_DIR)/db
LOCAL_DIR = $(DISTRIB_DIR)/local

ifndef PATH_CC
PATH_CC = gcc
endif

PATH_CFLAGS += -c -D_GNU_SOURCE
PATH_CFLAGS += -I. -I$(INCLUDE_DIR) -I$(DB_DIR) -I$(LOCAL_DIR) -I$(IX_DIR) -I$(EXP_DIR)
PATH_CFLAGS += -I$(CICAS_DIR)/exp
#PATH_CFLAGS += -g
			
ifndef PATH_AR
PATH_AR = ar
endif

AR_FLAGS += rv

ifndef PATH_LINK
PATH_LINK = gcc
endif

PATH_LDFLAGS += -L$(DB_DIR) -L$(LOCAL_DIR) -L$(IX_DIR)

LIBS = -lix -ldb_clts -llocals -lrt

EXEC = phases compare_ntcip compare_3418 phases_init_only

.c.o:	
	$(PATH_CC) $(CFLAGS) $(PATH_CFLAGS) $*.c -o $*.o 
.o:
	$(PATH_LINK) $(PATH_LDFLAGS) -o $@ $@.o $(LIBS) 

all: $(EXEC)

phases:		$(OBJS) phases.o
phases_init_only:		$(OBJS) phases_init_only.o
compare_ntcip:	$(OBJS) compare_ntcip.o
compare_3418:	$(OBJS) compare_3418.o

clean:
	rm -f *.o *.err *.a $(EXEC)

install:
	cp $(EXEC) $(ATSC_DIR)/test
