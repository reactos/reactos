#
# ReactOS test applications makefile
#

PATH_TO_TOP = ../..

include $(PATH_TO_TOP)/rules.mak


# Test applications
# alive apc args atomtest bench consume copymove count dump_shared_data
# event file gditest hello isotest lpc mstest mutex nptest
# pteb regtest sectest shm simple thread vmtest winhello
TEST_APPS = alive apc args atomtest bench consume copymove count dump_shared_data \
            event file gditest hello isotest lpc mstest mutex nptest \
            pteb regtest sectest shm simple thread tokentest vmtest winhello dibtest

TEST_MISC = 

all: $(TEST_APPS) $(TEST_MISC)

depends: 

implib: $(TEST_APPS:%=%_implib) \
	$(TEST_MISC:%=%_implib)

clean: 	$(TEST_APPS:%=%_clean) \
	$(TEST_MISC:%=%_clean)

install: $(TEST_APPS:%=%_install) \
	 $(TEST_MISC:%=%_install)

dist:	$(TEST_APPS:%=%_dist) \
	$(TEST_MISC:%=%_dist)

.PHONY: all depends implib clean install dist


#
# Test Applications
#
$(TEST_APPS): %:
	make -C $*

$(TEST_APPS:%=%_implib): %_implib:
	make -C $* implib

$(TEST_APPS:%=%_clean): %_clean:
	make -C $* clean

$(TEST_APPS:%=%_dist): %_dist:
	make -C $* dist

$(TEST_APPS:%=%_install): %_install:
	make -C $* install

.PHONY: $(TEST_APPS) $(TEST_APPS:%=%_implib) $(TEST_APPS:%=%_clean) $(TEST_APPS:%=%_install) $(TEST_APPS:%=%_dist)


#
# Misc Test Applications
#
$(TEST_MISC): %:
	make -C tests/$*

$(TEST_MISC:%=%_implib): %_implib:
	make -C tests/$* implib

$(TEST_MISC:%=%_clean): %_clean:
	make -C tests/$* clean

$(TEST_MISC:%=%_dist): %_dist:
	make -C tests/$* dist

$(TEST_MISC:%=%_install): %_install:
	make -C tests/$* install

.PHONY: $(TEST_MISC) $(TEST_MISC:%=%_implib) $(TEST_MISC:%=%_clean) $(TEST_MISC:%=%_install) $(TEST_MISC:%=%_dist)


etags:
	find . -name "*.[ch]" -print | etags --language=c -

# EOF

