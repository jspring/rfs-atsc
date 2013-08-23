# Makefile
#
# Copyright (c) 2008   Regents of the University of California
#
include $(CAPATH_MK_DEFS)

URMS_LIB = $(OBJDIR)/liburms_lib.a
URMS_OBJS = $(OBJDIR)/urms_lib.o

TOS_LIB = $(OBJDIR)/libtos_lib.a
TOS_OBJS = $(OBJDIR)/tos_lib.o

PATH_CFLAGS += -I$(ATSC_INC_DIR)
PATH_LIBS += -lurms_lib -ltos_lib -lab3418_lib -ldl
PATH_LDFLAGS += -L./lnx -L$(ATSC_LIB_DIR)

EXEC = $(OBJDIR)/urms $(OBJDIR)/urms_db_test $(OBJDIR)/send_urms_data


all: 	$(OBJDIR) $(URMS_LIB) $(TOS_LIB) $(EXEC)

$(URMS_LIB): $(URMS_OBJS)
	$(PATH_AR) $(PATH_AR_FLAGS) $@ $(URMS_OBJS)
#        cp $(URMS_LIB) $(SENS_LIB_DIR)/$(OBJDIR)

$(TOS_LIB): $(TOS_OBJS)
	$(PATH_AR) $(PATH_AR_FLAGS) $@ $(TOS_OBJS)
#        cp $(URMS_LIB) $(SENS_LIB_DIR)/$(OBJDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	\rm -f $(OBJDIR)/*.[oa] *.*~ $(EXEC)
