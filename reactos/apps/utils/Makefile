#
# ReactOS system utilities makefile
#

PATH_TO_TOP = ../..

include $(PATH_TO_TOP)/rules.mak


# Console system utilities
# cabman cat net objdir partinfo pice ps sc stats
UTIL_APPS = cat objdir partinfo sc stats

UTIL_NET_APPS = 


all: $(UTIL_APPS) $(UTIL_NET_APPS)

depends: 

implib: $(UTIL_APPS:%=%_implib) \
	$(UTIL_NET_APPS:%=%_implib)

clean: 	$(UTIL_APPS:%=%_clean) \
	$(UTIL_NET_APPS:%=%_clean)

install: $(UTIL_APPS:%=%_install) \
	 $(UTIL_NET_APPS:%=%_install)

dist:	$(UTIL_APPS:%=%_dist) \
	$(UTIL_NET_APPS:%=%_dist)

.PHONY: all depends implib clean install dist


#
# Utility Applications
#
$(UTIL_APPS): %:
	make -C $*

$(UTIL_APPS:%=%_implib): %_implib:
	make -C $* implib

$(UTIL_APPS:%=%_clean): %_clean:
	make -C $* clean

$(UTIL_APPS:%=%_dist): %_dist:
	make -C $* dist

$(UTIL_APPS:%=%_install): %_install:
	make -C $* install

.PHONY: $(UTIL_APPS) $(UTIL_APPS:%=%_implib) $(UTIL_APPS:%=%_clean) $(UTIL_APPS:%=%_install) $(UTIL_APPS:%=%_dist)


#
# GUI Utility Applications
#
$(UTIL_NET_APPS): %:
	make -C net/$*

$(UTIL_NET_APPS:%=%_implib): %_implib:
	make -C net/$* implib

$(UTIL_NET_APPS:%=%_clean): %_clean:
	make -C net/$* clean

$(UTIL_NET_APPS:%=%_dist): %_dist:
	make -C net/$* dist

$(UTIL_NET_APPS:%=%_install): %_install:
	make -C net/$* install

.PHONY: $(UTIL_NET_APPS) $(UTIL_NET_APPS:%=%_implib) $(UTIL_NET_APPS:%=%_clean) $(UTIL_NET_APPS:%=%_install) $(UTIL_NET_APPS:%=%_dist)


etags:
	find . -name "*.[ch]" -print | etags --language=c -

# EOF

