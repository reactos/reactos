#
# ReactOS test applications makefile
#

PATH_TO_TOP = ../..

include $(PATH_TO_TOP)/rules.mak

# test_old tests
TEST_APPS = accelerator alive alphablend apc args atomtest bench bitblt button \
button2 capclock carets combo consume copymove count dibtest dump_shared_data \
edit enumwnd enumws event global_mem gradient hello mdi \
hivetest icontest isotest lineclip linetest lpc \
mktime mstest multithrdwin multiwin mutex nptest patblt primitives pteb \
regtest sectest sertest shaptest shm statst statst2 stretchblt suspend \
tcpsvr terminate txtscale thread thread_msg tokentest user32 vmtest \
winhello wm_erasebkgnd wm_paint eventpair threadwait popupmenu \
map_dup_inherit p_dup_handle apc2 enhmetafile dirdlg guithreadinfo fiber
# Build failure:
# kernel32

TEST_MISC = 

all: $(TEST_APPS) $(TEST_MISC)

depends: 

implib: $(TEST_APPS:%=%_implib) \
	$(TEST_MISC:%=%_implib)

clean: 	$(TEST_APPS:%=%_clean) \
	$(TEST_MISC:%=%_clean)

install: $(TEST_APPS:%=%_install) \
	 $(TEST_MISC:%=%_install)

.PHONY: all depends implib clean install


#
# Test Applications
#
$(TEST_APPS): %:
	$(MAKE) -C $*

$(TEST_APPS:%=%_implib): %_implib:
	$(MAKE) -C $* implib

$(TEST_APPS:%=%_clean): %_clean:
	$(MAKE) -C $* clean

$(TEST_APPS:%=%_install): %_install:
	$(MAKE) -C $* install

.PHONY: $(TEST_APPS) $(TEST_APPS:%=%_implib) $(TEST_APPS:%=%_clean) $(TEST_APPS:%=%_install)


#
# Misc Test Applications
#
$(TEST_MISC): %:
	$(MAKE) -C tests/$*

$(TEST_MISC:%=%_implib): %_implib:
	$(MAKE) -C tests/$* implib

$(TEST_MISC:%=%_clean): %_clean:
	$(MAKE) -C tests/$* clean

$(TEST_MISC:%=%_install): %_install:
	$(MAKE) -C tests/$* install

.PHONY: $(TEST_MISC) $(TEST_MISC:%=%_implib) $(TEST_MISC:%=%_clean) $(TEST_MISC:%=%_install)


etags:
	find . -name "*.[ch]" -print | etags --language=c -

# EOF
