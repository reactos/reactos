#
# Global makefile for the ROSAPPS package
#

#
# Select your host
#
#HOST = djgpp-msdos
HOST = mingw32-windows

include rules.mak

#
# Available applications
#
APPS = 	calc \
	cmd \
	cmdutils \
	cmdutils\touch \
	dflat32 \
	hcalc \
	mc \
	notevil \
	sysutils \
	sysutils\regexpl \
	sysutils\tlist \
	net\arp \
	net\finger \
	net\ipconfig \
	net\ncftp \
	net\netstat \
	net\niclist \
	net\ping \
	net\telnet \
	net\whois \
	regedit \
	regedt32 \
	taskmgr \
	winfile

all: $(APPS)
.PHONY: all

clean: $(APPS:%=%_clean)
.PHONY: clean

floppy: make_floppy_dirs $(APPS:%=%_floppy)

dist: clean_dist_dir make_dist_dirs $(APPS:%=%_dist)
.PHONY: dist

#
# Applications
#
$(APPS): %:
	make -C $*

$(APPS:%=%_clean): %_clean:
	make -C $* clean

$(APPS:%=%_floppy): %_floppy:
	make -C $* floppy

$(APPS:%=%_dist): %_dist:
	make -C $* dist

.PHONY: $(APPS) $(APPS:%=%_clean) $(APPS:%=%_floppy) $(APPS:%=%_dist)


#
# Make an install floppy
#

install: all
	./install.sh /mnt/hda1

make_floppy_dirs:
ifeq ($(DOSCLI),yes)
	mkdir $(FLOPPY_DIR)\apps
else
	mkdir $(FLOPPY_DIR)/apps
endif

.PHONY: make_floppy_dirs

#
# Make a distribution saveset
#

clean_dist_dir:
ifeq ($(DOSCLI),yes)
#	$(RM) $(DIST_DIR)\apps\*.*
#	$(RMDIR) $(DIST_DIR)\apps
#	$(RMDIR) $(DIST_DIR)
else
	$(RM) -r $(DIST_DIR)
endif

make_dist_dirs:
ifeq ($(DOSCLI),yes)
	mkdir $(DIST_DIR)
	mkdir $(DIST_DIR)\apps
else
	mkdir $(DIST_DIR)
	mkdir $(DIST_DIR)/apps
endif

.PHONY: clean_dist_dir make_dist_dirs

# EOF
