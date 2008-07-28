# Makefile
#
# Copyright (c) 2008   Regents of the University of California
#
include $(CAPATH_MK_DEFS)

PATH_CFLAGS += -I$(ATSC_INC_DIR) -I$(ATSC_IX_DIR)
PATH_LDFLAGS += -L$(ATSC_IX_DIR)/$(OBJDIR)
PATH_LIBS += -lix 

EXEC = $(OBJDIR)/phases $(OBJDIR)/compare_ntcip \
	$(OBJDIR)/compare_3418 $(OBJDIR)/phases_init_only

all: 	$(OBJDIR) $(EXEC)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/phases:		$(OBJDIR)/phases.o
$(OBJDIR)/phases_init_only:	$(OBJDIR)/phases_init_only.o
$(OBJDIR)/compare_ntcip:	$(OBJDIR)/compare_ntcip.o
$(OBJDIR)/compare_3418:	$(OBJDIR)/compare_3418.o

clean:
	\rm -f $(OBJDIR)/*.o *.*~ $(EXEC)

install:
	cp $(EXEC) $(ATSC_DIR)/test

