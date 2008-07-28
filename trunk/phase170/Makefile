# Makefile
#
# Copyright (c) 2008   Regents of the University of California
#
include $(CAPATH_MK_DEFS)

PATH_CFLAGS += -I$(ATSC_INC_DIR) -I$(ATSC_IX_DIR) -I$(TSP_INC_DIR)
PATH_LDFLAGS += -L$(ATSC_IX_DIR)/$(OBJDIR)
PATH_LIBS += -lix 

EXEC = $(OBJDIR)/phase170e 

all: 	$(OBJDIR) $(EXEC)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/phase170e:		$(OBJDIR)/phase170e.o

clean:
	\rm -f $(OBJDIR)/*.o *.*~ $(EXEC)

install:
	cp $(EXEC) $(ATSC_DIR)/test
