DISPLAY_DRIVERS = vga
MINIPORT_DRIVERS = vga vbe

all: $(DISPLAY_DRIVERS:%=DD%) $(MINIPORT_DRIVERS:%=MP%)

implib: $(DISPLAY_DRIVERS:%=DD%_implib) $(MINIPORT_DRIVERS:%=MP%_implib)

clean: $(DISPLAY_DRIVERS:%=DD%_clean) $(MINIPORT_DRIVERS:%=MP%_clean)

install: $(DISPLAY_DRIVERS:%=DD%_install) $(MINIPORT_DRIVERS:%=MP%_install)

bootcd: $(DISPLAY_DRIVERS:%=DD%_bootcd) $(MINIPORT_DRIVERS:%=MP%_bootcd)

#
# Video display driver rules
#

$(DISPLAY_DRIVERS:%=DD%): DD%:
	$(MAKE) -C displays/$*

$(DISPLAY_DRIVERS:%=DD%_implib): DD%_implib:
	$(MAKE) -C displays/$* implib

$(DISPLAY_DRIVERS:%=DD%_clean): DD%_clean:
	$(MAKE) -C displays/$* clean

$(DISPLAY_DRIVERS:%=DD%_install): DD%_install:
	$(MAKE) -C displays/$* install

$(DISPLAY_DRIVERS:%=DD%_bootcd): DD%_bootcd:
	$(MAKE) -C displays/$* bootcd

.PHONY: $(DISPLAY_DRIVERS:%=DD%) $(DISPLAY_DRIVERS:%=DD%_implib) $(DISPLAY_DRIVERS:%=DD%_clean)\
        $(DISPLAY_DRIVERS:%=DD%_install) $(DISPLAY_DRIVERS:%=DD%_bootcd)


#
# Video miniport driver rules
#

$(MINIPORT_DRIVERS:%=MP%): MP%:
	$(MAKE) -C miniport/$*

$(MINIPORT_DRIVERS:%=MP%_implib): MP%_implib:
	$(MAKE) -C miniport/$* implib

$(MINIPORT_DRIVERS:%=MP%_clean): MP%_clean:
	$(MAKE) -C miniport/$* clean

$(MINIPORT_DRIVERS:%=MP%_install): MP%_install:
	$(MAKE) -C miniport/$* install

$(MINIPORT_DRIVERS:%=MP%_bootcd): MP%_bootcd:
	$(MAKE) -C miniport/$* bootcd

.PHONY: $(MINIPORT_DRIVERS:%=MP%) $(MINIPORT_DRIVERS:%=MP%_implib) $(MINIPORT_DRIVERS:%=MP%_clean)\
        $(MINIPORT_DRIVERS:%=MP%_install) $(MINIPORT_DRIVERS:%=MP%_bootcd)
