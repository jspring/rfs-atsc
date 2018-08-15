# Makefile
# Took out build for second executable with -DCOMPARE
#
# Copyright (c) 2008   Regents of the University of California
#
include $(CAPATH_MK_DEFS)

SET_MIN_MAX_GREEN_LIB = $(OBJDIR)/libset_min_max_green_lib.a
SET_MIN_MAX_GREEN_OBJS = $(OBJDIR)/set_min_max_green_lib.o

SERVER_LIB = $(OBJDIR)/libserver_lib.a
SERVER_LIB_OBJS = $(OBJDIR)/server_lib.o

AB3418_LIB = $(OBJDIR)/libab3418_lib.a
AB3418_LIB_OBJS = $(OBJDIR)/ab3418_lib.o

PATH_LIBS += -lset_min_max_green_lib -lserver_lib -lab3418_lib -ldl
PATH_LDFLAGS += -L./lnx
PATH_CFLAGS += -I$(ATSC_INC_DIR) -pg

HDRS = fcs.h msgs.h ab3418comm.h ab3418_lib.h server_lib.h set_min_max_green.h

EXEC =  $(OBJDIR)/rd3418  $(OBJDIR)/server $(OBJDIR)/client $(OBJDIR)/ab3418comm $(OBJDIR)/set_min_max_green_db_test $(OBJDIR)/tscp_memory_probe $(OBJDIR)/receive_urms_data $(OBJDIR)/send_char_to_db_var

all: $(OBJDIR) $(SET_MIN_MAX_GREEN_LIB) $(SERVER_LIB) $(AB3418_LIB) $(EXEC) 

$(SET_MIN_MAX_GREEN_LIB): $(SET_MIN_MAX_GREEN_OBJS)
	$(PATH_AR) $(PATH_AR_FLAGS) $@ $(SET_MIN_MAX_GREEN_OBJS)

$(SERVER_LIB): $(SERVER_LIB_OBJS)
	$(PATH_AR) $(PATH_AR_FLAGS) $@ $(SERVER_LIB_OBJS)

$(AB3418_LIB): $(AB3418_LIB_OBJS)
	$(PATH_AR) $(PATH_AR_FLAGS) $@ $(AB3418_LIB_OBJS)

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	\rm -f $(OBJDIR)/*.[oa] *.*~ $(EXEC) $(ALGORITHM_DIR)/$(OBJDIR)/*.[oa]
