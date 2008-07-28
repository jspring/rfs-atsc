# Makefile
#
# Copyright (c) 2006   Regents of the University of California
#
include $(CAPATH_MK_DEFS)

PATH_CFLAGS += -I$(ATSC_INC_DIR)
PATH_LIBS += -lusb
PATH_LDFLAGS += -Llnx
RD_OBJS = $(OBJDIR)/atsc_dio.o $(OBJDIR)/atsc_db.o
TEST_OBJS = $(OBJDIR)/atsc_fake.o $(OBJDIR)/atsc_db.o

DIGIO_LIB = $(OBJDIR)/libusb_digio.a

EXEC = $(OBJDIR)/rdsniff $(OBJDIR)/testsniff 

all: $(OBJDIR) $(DIGIO_LIB) $(RD_OBJS) $(TEST_OBJS) $(EXEC)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(DIGIO_LIB): $(OBJDIR)/usb_digio.o 
	$(PATH_AR) $(PATH_AR_FLAGS) $@ $(OBJDIR)/usb_digio.o

$(OBJDIR)/rdsniff: $(RD_OBJS)	
	$(PATH_LINK) $(LDFLAGS) $(PATH_LDFLAGS) -o $@ $(RD_OBJS) $(PATH_LIBS) -lusb_digio

$(OBJDIR)/testsniff: $(TEST_OBJS)	
	$(PATH_LINK) $(LDFLAGS) $(PATH_LDFLAGS) -o $@ $(TEST_OBJS) $(PATH_LIBS)

clean:
	rm -f $(OBJDIR)/*.o *.err *.a $(EXEC)

install:
	cp $(EXEC) $(ATSC_DIR)/test
