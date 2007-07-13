# Makefile
#
# Copyright (c) 2006   Regents of the University of California
#

ATSC_DIR = ..
DISTRIB_DIR = /home/path

INCLUDE_DIR = $(ATSC_DIR)/include
LOCAL_DIR = $(DISTRIB_DIR)/local
DB_DIR = $(DISTRIB_DIR)/db

CC = cc
CFLAGS += -c -D_GNU_SOURCE
CFLAGS += -I. -I$(LOCAL_DIR) -I$(DB_DIR) -I$(INCLUDE_DIR)
CFLAGS += -g
			
AR = ar
AR_FLAGS += rv

LINK = cc
LFLAGS += -L$(LOCAL_DIR) -L$(DB_DIR)

LIBS = -ldb_clts -llocals -lrt -lusb

EXEC = rdsniff testsniff 

.c.o:	
	$(CC) $(CFLAGS) $*.c -o $*.o 
.o:
	$(LINK) $(LFLAGS) -o $@ $@.o $(LIBS) 

all: libusb_digio.a $(EXEC)

libusb_digio.a: usb_digio.o 
	$(AR) $(AR_FLAGS) $@ usb_digio.o

rdsniff:	atsc_dio.o atsc_db.o
	$(LINK) $(LFLAGS) -o $@ atsc_dio.o atsc_db.o $(LIBS) libusb_digio.a

testsniff:	atsc_fake.o atsc_db.o
	$(LINK) $(LFLAGS) -o $@ atsc_fake.o atsc_db.o $(LIBS) 

clean:
	rm -f *.o *.err *.a $(EXEC)

install:
	cp $(EXEC) $(ATSC_DIR)/test

