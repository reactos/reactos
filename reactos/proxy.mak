MAKEFLAGS += --no-print-directory --silent

$(DEFAULT):
	@$(MAKE) -C $(TOP) $(DEFAULT)

all:
	@$(MAKE) -C $(TOP) all

clean:
	@$(MAKE) -C $(TOP) $(DEFAULT)_clean

$(DEFAULT)_clean: clean
