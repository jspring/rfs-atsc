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
TSP_DIR = /home/tsp
CICAS_DIR = /home/cicas
IX_DIR = $(ATSC_DIR)/ix
SENS_DIR = $(DISTRIB_DIR)/sens
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
PATH_CFLAGS += -I$(SENS_DIR)/sms
PATH_CFLAGS += -I$(TSP_DIR)/src
PATH_CFLAGS += -g
			
ifndef PATH_AR
PATH_AR = ar
endif

AR_FLAGS += rv

ifndef PATH_LINK
PATH_LINK = gcc
endif

PATH_LDFLAGS += -L$(DB_DIR) -L$(LOCAL_DIR) -L$(IX_DIR)

LIBS = -lix -ldb_clts -llocals -lrt

EXEC = phase170e 

.c.o:	
	$(PATH_CC) $(CFLAGS) $(PATH_CFLAGS) $*.c -o $*.o 
.o:
	$(PATH_LINK) $(PATH_LDFLAGS) -o $@ $@.o $(LIBS) 

all: $(EXEC)

phase170e:		$(OBJS) phase170e.o

clean:
	rm -f *.o *.err *.a $(EXEC)

install:
	cp $(EXEC) $(ATSC_DIR)/test
