# Makefile
# Took out build for special version used for comparing latency.
# Use compiler flag -DCOMPARE if it is wanted.
#
# Copyright (c) 2006   Regents of the University of California
#
include $(CAPATH_MK_DEFS)

PATH_CFLAGS += -I$(ATSC_INC_DIR)
PATH_OBJS = $(OBJDIR)/asc_get.o $(OBJDIR)/parse_asc.o \
		$(OBJDIR)/format_asc.o $(OBJDIR)/fcs.o

HDRS = atsc.h mibdefs.h 
EXEC = $(OBJDIR)/rdntcip 

all: 	$(OBJDIR) $(PATH_OBJS) $(EXEC)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/rdntcip:
	$(PATH_LINK) $(LDFLAGS) $(PATH_LDFLAGS) -o $@ $(PATH_OBJS) $(PATH_LIBS)
clean:
	\rm -f $(OBJDIR)/*.o *.*~ $(EXEC)

install:
	cp $(EXEC) $(ATSC_DIR)/test
