#
# ReactOS test applications makefile
#

PATH_TO_TOP = ../..

include $(PATH_TO_TOP)/rules.mak

# test_old tests
TEST_APPS = SampleWindow alive apc args atomtest bench bitblt button \
button2 capclock carets cliarea combo consume copymove count dibtest \
dump_shared_data edit enumwnd event file gditest global_mem hello \
hivetest icontest isotest lineclip linetest lock lpc messagebox mktime \
mstest multiwin mutex nptest patblt pipe primitives pteb regtest \
sectest sertest shaptest shm statst statst2 stretchblt suspend \
tcpsvr terminate txtscale thread thread_msg tokentest vmtest \
winhello winhello2 wm_erasebkgnd wm_paint eventpair threadwait \
map_dup_inherit

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
	$(MAKE) -C $*

$(TEST_APPS:%=%_implib): %_implib:
	$(MAKE) -C $* implib

$(TEST_APPS:%=%_clean): %_clean:
	$(MAKE) -C $* clean

$(TEST_APPS:%=%_dist): %_dist:
	$(MAKE) -C $* dist

$(TEST_APPS:%=%_install): %_install:
	$(MAKE) -C $* install

.PHONY: $(TEST_APPS) $(TEST_APPS:%=%_implib) $(TEST_APPS:%=%_clean) $(TEST_APPS:%=%_install) $(TEST_APPS:%=%_dist)


#
# Misc Test Applications
#
$(TEST_MISC): %:
	$(MAKE) -C tests/$*

$(TEST_MISC:%=%_implib): %_implib:
	$(MAKE) -C tests/$* implib

$(TEST_MISC:%=%_clean): %_clean:
	$(MAKE) -C tests/$* clean

$(TEST_MISC:%=%_dist): %_dist:
	$(MAKE) -C tests/$* dist

$(TEST_MISC:%=%_install): %_install:
	$(MAKE) -C tests/$* install

.PHONY: $(TEST_MISC) $(TEST_MISC:%=%_implib) $(TEST_MISC:%=%_clean) $(TEST_MISC:%=%_install) $(TEST_MISC:%=%_dist)


etags:
	find . -name "*.[ch]" -print | etags --language=c -

# EOF

