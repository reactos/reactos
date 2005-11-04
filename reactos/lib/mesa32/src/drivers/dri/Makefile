# src/mesa/drivers/dri/Makefile

TOP = ../../../..

include $(TOP)/configs/current



default: $(LIB_DIR) subdirs


$(LIB_DIR):
	-mkdir $(LIB_DIR)


subdirs:
	echo $(DRI_DIRS)
	@for dir in $(DRI_DIRS) ; do \
		echo $$dir ; \
		(cd $$dir ; $(MAKE)) || exit 1; \
	done


install:
	@for dir in $(DRI_DIRS) ; do \
		(cd $$dir ; $(MAKE) install) || exit 1; \
	done


clean:
	@for dir in $(DRI_DIRS) ; do \
		(cd $$dir ; $(MAKE) clean) ; \
	done
	-rm -f common/*.o
