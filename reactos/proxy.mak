MAKEFLAGS += --no-print-directory

$(DEFAULT):
	@$(MAKE) -C $(TOP) $(DEFAULT)

all:
	@$(MAKE) -C $(TOP) all

clean:
	@$(MAKE) -C $(TOP) $(DEFAULT)_clean

$(DEFAULT)_clean: clean
