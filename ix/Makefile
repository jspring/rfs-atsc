# Makefile, updated for portability
#
# Copyright (c) 2008   Regents of the University of California
#
include $(CAPATH_MK_DEFS)

PATH_CFLAGS += -I$(ATSC_INC_DIR)
PATH_CFLAGS += -I$(TSP_INC_DIR)
PATH_LDFLAGS += -L$(OBJDIR)
#PATH_LIBS += -lix -lsqlite3
PATH_LIBS += -lix

HDRS = ix_msg.h

IX_LIB = $(OBJDIR)/libix.a
IX_OBJS = $(OBJDIR)/ix_utils.o $(OBJDIR)/ix_db_utils.o $(OBJDIR)/ix_file_utils.o

EXEC = $(OBJDIR)/rcv_ix_msg $(OBJDIR)/snd_ix_msg $(OBJDIR)/ix_byte_count

all: $(OBJDIR) $(IX_LIB) $(EXEC)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(IX_LIB): $(IX_OBJS)
	$(PATH_AR) $(PATH_AR_FLAGS) $@ $(IX_OBJS)

$(OBJDIR)/rcv_ix_msg:	$(OBJDIR)/rcv_ix_msg.o
$(OBJDIR)/snd_ix_msg:	$(OBJDIR)/snd_ix_msg.o
$(OBJDIR)/ix_byte_count:	$(OBJDIR)/ix_byte_count.o

clean:
	rm -f $(OBJDIR)/*.o *.err $(OBJDIR)/*.a $(EXEC)

install:
	cp $(EXEC) $(ATSC_DIR)/test
