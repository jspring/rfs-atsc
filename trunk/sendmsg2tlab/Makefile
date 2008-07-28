# Makefile
#
# Copyright (c) 2006   Regents of the University of California
#
include $(CAPATH_MK_DEFS)

PATH_CFLAGS += -I$(ATSC_INC_DIR) -I$(ATSC_IX_DIR)
PATH_LDFLAGS += -L$(ATSC_IX_DIR)/$(OBJDIR)
PATH_LIBS += -lix

EXEC = $(OBJDIR)/sendatsc2tlab

all: $(OBJDIR) $(EXEC)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/sendatsc2tlab:	$(OBJDIR)/sendatsc2tlab.o

clean:
	rm -f $(OBJDIR)/*.o *.err *.a $(EXEC)

install:
	cp $(EXEC) $(ATSC_DIR)/test
