#
# ReactOS test applications makefile
#

PATH_TO_TOP = ../../reactos

include $(PATH_TO_TOP)/rules.mak

TEST_APPS = \
	accelerator \
	alive \
	alphablend \
	apc \
	apc2 \
	args \
	atomtest \
	bench \
	bitblt \
	button \
	button2 \
	capclock \
	carets \
	combo \
	consume \
	copymove \
	count \
	dibtest \
	dirdlg \
	dump_shared_data \
	edit \
	enhmetafile \
	enumwnd \
	enumws \
	event \
	eventpair \
	fiber \
	global_mem \
	gradient \
	guithreadinfo \
	hello \
	mdi \
	hivetest \
	icontest \
	isotest \
	lineclip \
	linetest \
	lpc \
	map_dup_inherit \
	mktime \
	mstest \
	multithrdwin \
	multiwin \
	mutex \
	nptest \
	patblt \
	popupmenu \
	primitives \
	pteb \
	p_dup_handle \
	regtest \
	sectest \
	sertest \
	shaptest \
	shm \
	statst \
	statst2 \
	stretchblt \
	suspend \
	tcpsvr \
	terminate \
	txtscale \
	thread \
	thread_msg \
	threadwait \
	tokentest \
	vmtest \
	winhello \
	wm_erasebkgnd \
	wm_paint

all: $(TEST_APPS)

depends: 

implib: $(TEST_APPS:%=%_implib)

clean: 	$(TEST_APPS:%=%_clean)

install: $(TEST_APPS:%=%_install)

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

etags:
	find . -name "*.[ch]" -print | etags --language=c -

# EOF
