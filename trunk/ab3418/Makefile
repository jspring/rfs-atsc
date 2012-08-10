# Makefile
# Took out build for second executable with -DCOMPARE
#
# Copyright (c) 2008   Regents of the University of California
#
include $(CAPATH_MK_DEFS)

PATH_CFLAGS += -I$(ATSC_INC_DIR)

HDRS = fcs.h msgs.h 
EXEC = $(OBJDIR)/rd3418  $(OBJDIR)/set_max_green

all: 	$(OBJDIR) $(EXEC)

$(OBJDIR):
	mkdir -p $(OBJDIR)

#$(OBJDIR)/rd3418: $(OBJDIR)/rd3418.o
#$(OBJDIR)/set_max_green: $(OBJDIR)/set_max_green.o

clean:
	\rm -f $(OBJDIR)/*.o *.*~ $(EXEC)

install:
	cp $(EXEC) $(ATSC_DIR)/test

