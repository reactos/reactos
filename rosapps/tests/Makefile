#
# ReactOS test applications makefile
#

PATH_TO_TOP = ../../reactos

include $(PATH_TO_TOP)/rules.mak

# Broken tests 
# dnsapi iptest lan regdump subclass
# polytest - Needs a mingw makefile

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
create-links \
dibtest \
dirdlg \
diskspeed \
dnsquery \
edit \
enhmetafile \
enumfonts \
enumwnd \
enumws \
event \
eventpair \
fiber \
global_mem \
gradient \
guithreadinfo \
hello \
hivetest \
icontest \
isotest \
lineclip \
linetest \
lpc \
map_dup_inherit \
mdi \
mktime \
moztest \
mstest \
multithrdwin \
multiwin \
mutex \
nameserverlist \
noexecute \
nptest \
oskittcp \
p_dup_handle \
palbitblt \
patblt \
popupmenu \
primitives \
pteb \
regqueryvalue \
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
thread \
thread_msg \
threadwait \
tmrqueue \
tokentest \
txtscale \
vmtest \
winhello \
wm_erasebkgnd \
wm_paint \
zwcontinue

# Deactivated tests:
# dump_shared_data
# miditest

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
