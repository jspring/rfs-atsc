# Makefile for ATSC (Actuated Traffic Signal Controller) code
# This code obtains signal phase and timing through a variety
# of methods and supplies a countdown that can be sent out in
# a UDP message.
#
# Copyright (c) 2008   Regents of the University of California
#

include $(CAPATH_MK_DEFS)

ATSC_LIBRARIES = ix/$(OBJDIR)/libix.a

all: $(ATSC_LIBRARIES) 
	(cd ab3418; make; make install)
	(cd ntcip; make; make install)
	(cd phases; make; make install)
	(cd phase170; make; make install)
	(cd sendmsg2tlab; make; make install)

ix/$(OBJDIR)/libix.a: $(wildcard ix/*.[ch])
	(cd $(LOCAL_DIR); make; make install)

# separate target, not part of "all", because can only 
# be done if usb_digio library is available
sniff: 
	(cd sniffer; make; make install)
	echo "Don't forget to set sniffer suid root"

clean:
	(cd sniffer; make clean)
	(cd ab3418; make clean)
	(cd ntcip; make clean)
	(cd phases; make clean)
	(cd phase170; make clean)
	(cd sendmsg2tlab; make clean)
