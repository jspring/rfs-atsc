# Makefile
#
# Copyright (c) 2006   Regents of the University of California
#
include $(CAPATH_MK_DEFS)

PATH_CFLAGS += -I$(ATSC_INC_DIR)
PATH_LDFLAGS += -Llnx
PATH_OBJS = $(OBJDIR)/atsc_db.o

EXEC = $(OBJDIR)/atsc_edi 

all: $(OBJDIR) $(EXEC)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/atsc_edi: $(PATH_OBJS)	

clean:
	rm -f $(OBJDIR)/*.o *.err *.a $(EXEC)

install:
	cp $(EXEC) $(ATSC_DIR)/test
