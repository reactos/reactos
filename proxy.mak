MAKEFLAGS += --no-print-directory

$(DEFAULT):
	@$(MAKE) -C $(TOP) $(DEFAULT)

all:
	@$(MAKE) -C $(TOP) all

depends:
	@$(MAKE) -C $(TOP) $(DEFAULT)_depends

install:
	@$(MAKE) -C $(TOP) $(DEFAULT)_install

clean:
	@$(MAKE) -C $(TOP) $(DEFAULT)_clean

test:
	@$(MAKE) -C $(TOP) $(DEFAULT)_test

$(DEFAULT)_clean: clean
