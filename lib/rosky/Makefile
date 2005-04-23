#
# ReactOS SkyOS implementation
#

PATH_TO_TOP = ../..

include $(PATH_TO_TOP)/rules.mak

SKY_LIBS = libsky libskygi

all: $(SKY_LIBS)

depends: 

implib: $(SKY_LIBS:%=%_implib)

test: $(SKY_LIBS:%=%_test)

clean: 	$(SKY_LIBS:%=%_clean)

install: $(SKY_LIBS:%=%_install)

bootcd: $(SKY_LIBS:%=%_bootcd)

.PHONY: all depends implib clean install bootcd


#
# Sky Libs
#
$(SKY_LIBS): %:
	$(MAKE) -C $*

$(SKY_LIBS:%=%_implib): %_implib:
	$(MAKE) -C $* implib

$(SKY_LIBS:%=%_test): %_test:
	$(MAKE) -C $* test

$(SKY_LIBS:%=%_clean): %_clean:
	$(MAKE) -C $* clean

$(SKY_LIBS:%=%_install): %_install:
	$(MAKE) -C $* install

$(SKY_LIBS:%=%_bootcd): %_bootcd:
	$(MAKE) -C $* bootcd

.PHONY: $(SKY_LIBS) $(SKY_LIBS:%=%_implib) $(SKY_LIBS:%=%_test) \
        $(SKY_LIBS:%=%_clean) $(SKY_LIBS:%=%_install) \
        $(SKY_LIBS:%=%_bootcd)


etags:
	find . -name "*.[ch]" -print | etags --language=c -
