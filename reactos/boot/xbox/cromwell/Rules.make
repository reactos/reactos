#
# This file contains rules which are shared between multiple Makefiles.
#

EXTRA_CFLAGS += -I$(TOPDIR)/include -I$(TOPDIR)/fs/cdrom -I$(TOPDIR)/drivers/video -I$(TOPDIR)/lib/eeprom -I$(TOPDIR)/lib/misc

#
# False targets.
#
.PHONY: dummy

CFLAGSBR = -Wall -Werror 

#
# Special variables which should not be exported
#
unexport subdirs

comma	:= ,

#
# Get things started.
#
first_rule: sub_dirs
	$(MAKE) all_targets

SUB_DIRS	:= $(subdir)

#
# Common rules
#

BootPerformPicChallengeResponseAction.o: BootPerformPicChallengeResponseAction.c
	$(CC) $(CFLAGSBR) $(INCLUDE) -o $(TOPDIR)/obj/$@ -c $<

%.o     : %.c
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -o $(TOPDIR)/obj/$@ -c $<

%.o     : %.S
	$(CC) -DASSEMBLER $(CFLAGS) $(EXTRA_CFLAGS) -o $(TOPDIR)/obj/$@ -c $<

all_targets: $(O_TARGET)

$(O_TARGET): $(obj)

#
# A rule to make subdirectories
#
subdir-list = $(sort $(patsubst %,_subdir_%,$(SUB_DIRS)))
sub_dirs: dummy $(subdir-list)

ifdef SUB_DIRS
$(subdir-list) : dummy
	$(MAKE) -C $(patsubst _subdir_%,%,$@)
endif

#
# A rule to do nothing
#
dummy:

#
# This is useful for testing
#
script:
	$(SCRIPT)
