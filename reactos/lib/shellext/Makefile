#
# ReactOS shell extensions makefile
#

PATH_TO_TOP = ../..

include $(PATH_TO_TOP)/rules.mak

SHELL_EXTENSIONS = slayer

all: $(SHELL_EXTENSIONS)

depends: 

implib: $(SHELL_EXTENSIONS:%=%_implib)

test: $(SHELL_EXTENSIONS:%=%_test)

clean: 	$(SHELL_EXTENSIONS:%=%_clean)

install: $(SHELL_EXTENSIONS:%=%_install)

bootcd: $(SHELL_EXTENSIONS:%=%_bootcd)

.PHONY: all depends implib clean install bootcd


#
# Shell extensions
#
$(SHELL_EXTENSIONS): %:
	$(MAKE) -C $*

$(SHELL_EXTENSIONS:%=%_implib): %_implib:
	$(MAKE) -C $* implib

$(SHELL_EXTENSIONS:%=%_test): %_test:
	$(MAKE) -C $* test

$(SHELL_EXTENSIONS:%=%_clean): %_clean:
	$(MAKE) -C $* clean

$(SHELL_EXTENSIONS:%=%_install): %_install:
	$(MAKE) -C $* install

$(SHELL_EXTENSIONS:%=%_bootcd): %_bootcd:
	$(MAKE) -C $* bootcd

.PHONY: $(SHELL_EXTENSIONS) $(SHELL_EXTENSIONS:%=%_implib) $(SHELL_EXTENSIONS:%=%_test) \
        $(SHELL_EXTENSIONS:%=%_clean) $(SHELL_EXTENSIONS:%=%_install) \
        $(SHELL_EXTENSIONS:%=%_bootcd)


etags:
	find . -name "*.[ch]" -print | etags --language=c -
