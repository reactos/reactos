# src/mesa/drivers/dri/Makefile

TOP = ../../../..

include $(TOP)/configs/current



default: $(TOP)/$(LIB_DIR) subdirs


$(TOP)/$(LIB_DIR):
	-mkdir $(TOP)/$(LIB_DIR)


subdirs:
	@for dir in $(DRI_DIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE)) || exit 1 ; \
		fi \
	done


install:
	@for dir in $(DRI_DIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE) install) || exit 1 ; \
		fi \
	done


clean:
	@for dir in $(DRI_DIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE) clean) ; \
		fi \
	done
	-rm -f common/*.o
