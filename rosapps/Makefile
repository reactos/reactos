#
# Global makefile for the ROSAPPS package
#

include rules.mak

#
# Available applications
#
#net$(SEP)arp
#net$(SEP)ipconfig
#net$(SEP)netstat
#net$(SEP)niclist
#net$(SEP)telnet
#net$(SEP)whois
APPS = 	calc \
	cmdutils \
	cmdutils$(SEP)mode \
	dflat32 \
	hcalc \
	mc \
	notevil \
	sysutils \
	sysutils$(SEP)ctm \
	sysutils$(SEP)tlist \
	net$(SEP)finger \
	net$(SEP)ncftp \
	net$(SEP)ping \
	regedit \
	regedt32 \
	welcome \
	winfile

# Removed due to dependance on C++ compiler
#
#	taskmgr \
#	sysutils$(SEP)regexpl \
#

all: $(APPS)
.PHONY: all

implib:
.PHONY: implib

clean: $(APPS:%=%_clean)
.PHONY: clean

install: $(APPS:%=%_install)
.PHONY: install

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

$(APPS:%=%_install): %_install:
	make -C $* install

.PHONY: $(APPS) $(APPS:%=%_clean) $(APPS:%=%_floppy) $(APPS:%=%_dist) $(APPS:%=%_install)


#
# Make an install floppy
#

#install: all
#	./install.sh /mnt/hda1

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

#dist: $(TOOLS_PATH)/rcopy$(EXE_POSTFIX) dist_clean dist_dirs \
#      $(WINE_MODULES:%=%_dist)

.PHONY: clean_dist_dir make_dist_dirs

# EOF
