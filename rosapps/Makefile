#
# Global makefile for the ROSAPPS package
#

include rules.mak

#
# Available applications
#
 
APPS = 	calc \
	cmdutils \
	cmdutils$(SEP)mode \
	dflat32 \
	hcalc \
	mc \
	notepad \
	notevil \
	sysutils \
	demos$(SEP)maze\
	sysutils$(SEP)ctm \
	net$(SEP)ping \
	net$(SEP)finger \
	net$(SEP)telnet \
	net$(SEP)niclist \
	net$(SEP)whois \
	net$(SEP)arp \
	net$(SEP)ncftp \
        net$(SEP)netstat \
	net$(SEP)ipconfig \
	regedt32 \
	welcome \
	winfile

# Removed due to dependance on C++ compiler
#
#	taskmgr \
#	sysutils$(SEP)regexpl \
#
# Removed because it doesn't build correctly
#
#	sysutils$(SEP)tlist \
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
	$(MAKE)  -C $*

$(APPS:%=%_clean): %_clean:
	$(MAKE)  -C $* clean

$(APPS:%=%_floppy): %_floppy:
	$(MAKE)  -C $* floppy

$(APPS:%=%_dist): %_dist:
	$(MAKE)  -C $* dist

$(APPS:%=%_install): %_install:
	$(MAKE)  -C $* install

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
