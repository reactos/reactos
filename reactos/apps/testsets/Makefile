#
# ReactOS system testsets makefile
#

PATH_TO_TOP = ../..

include $(PATH_TO_TOP)/rules.mak


# Testset applications
TEST_SETS = loadlib

TEST_KERNEL32 = 

TEST_MSVCRT = 

TEST_COM = 

TEST_SEH = 

TEST_REGRESSIONS = 

all: $(TEST_SETS)  $(TEST_KERNEL32) $(TEST_MSVCRT) $(TEST_COM) $(TEST_SEH) $(TEST_REGRESSIONS) 

depends: 

implib: $(TEST_SETS:%=%_implib) \
	$(TEST_KERNEL32:%=%_implib) \
	$(TEST_MSVCRT:%=%_implib) \
	$(TEST_COM:%=%_implib) \
	$(TEST_SEH:%=%_implib) \
	$(TEST_REGRESSIONS:%=%_implib)

clean: 	$(TEST_SETS:%=%_clean) \
	$(TEST_KERNEL32:%=%_clean) \
	$(TEST_MSVCRT:%=%_clean) \
	$(TEST_COM:%=%_clean) \
	$(TEST_SEH:%=%_clean) \
	$(TEST_REGRESSIONS:%=%_clean)

install: $(TEST_SETS:%=%_install) \
	 $(TEST_KERNEL32:%=%_install) \
	 $(TEST_MSVCRT:%=%_install) \
	 $(TEST_COM:%=%_install) \
	 $(TEST_SEH:%=%_install) \
	 $(TEST_REGRESSIONS:%=%_install)

dist:	$(TEST_SETS:%=%_dist) \
	$(TEST_KERNEL32:%=%_dist) \
	$(TEST_MSVCRT:%=%_dist) \
	$(TEST_COM:%=%_dist) \
	$(TEST_SEH:%=%_dist) \
	$(TEST_REGRESSIONS:%=%_dist)

.PHONY: all depends implib clean install dist


#
# Testset Applications
#
$(TEST_SETS): %:
	$(MAKE) -C $*

$(TEST_SETS:%=%_implib): %_implib:
	$(MAKE) -C $* implib

$(TEST_SETS:%=%_clean): %_clean:
	$(MAKE) -C $* clean

$(TEST_SETS:%=%_dist): %_dist:
	$(MAKE) -C $* dist

$(TEST_SETS:%=%_install): %_install:
	$(MAKE) -C $* install

.PHONY: $(TEST_SETS) $(TEST_SETS:%=%_implib) $(TEST_SETS:%=%_clean) $(TEST_SETS:%=%_install) $(TEST_SETS:%=%_dist)


#
# Kernel32 Test Applications
#
$(TEST_KERNEL32): %:
	$(MAKE) -C kernel32/$*

$(TEST_KERNEL32:%=%_implib): %_implib:
	$(MAKE) -C kernel32/$* implib

$(TEST_KERNEL32:%=%_clean): %_clean:
	$(MAKE) -C kernel32/$* clean

$(TEST_KERNEL32:%=%_dist): %_dist:
	$(MAKE) -C kernel32/$* dist

$(TEST_KERNEL32:%=%_install): %_install:
	$(MAKE) -C kernel32/$* install

.PHONY: $(TEST_KERNEL32) $(TEST_KERNEL32:%=%_implib) $(TEST_KERNEL32:%=%_clean) $(TEST_KERNEL32:%=%_install) $(TEST_KERNEL32:%=%_dist)


#
# msvcrt Test Applications
#
$(TEST_MSVCRT): %:
	$(MAKE) -C msvcrt/$*

$(TEST_MSVCRT:%=%_implib): %_implib:
	$(MAKE) -C msvcrt/$* implib

$(TEST_MSVCRT:%=%_clean): %_clean:
	$(MAKE) -C msvcrt/$* clean

$(TEST_MSVCRT:%=%_dist): %_dist:
	$(MAKE) -C msvcrt/$* dist

$(TEST_MSVCRT:%=%_install): %_install:
	$(MAKE) -C msvcrt/$* install

.PHONY: $(TEST_MSVCRT) $(TEST_MSVCRT:%=%_implib) $(TEST_MSVCRT:%=%_clean) $(TEST_MSVCRT:%=%_install) $(TEST_MSVCRT:%=%_dist)


#
# COM Test Applications
#
$(TEST_COM): %:
	$(MAKE) -C com/$*

$(TEST_COM:%=%_implib): %_implib:
	$(MAKE) -C com/$* implib

$(TEST_COM:%=%_clean): %_clean:
	$(MAKE) -C com/$* clean

$(TEST_COM:%=%_dist): %_dist:
	$(MAKE) -C com/$* dist

$(TEST_COM:%=%_install): %_install:
	$(MAKE) -C com/$* install

.PHONY: $(TEST_COM) $(TEST_COM:%=%_implib) $(TEST_COM:%=%_clean) $(TEST_COM:%=%_install) $(TEST_COM:%=%_dist)


#
# SEH Test Applications
#
$(TEST_SEH): %:
	$(MAKE) -C seh/$*

$(TEST_SEH:%=%_implib): %_implib:
	$(MAKE) -C seh/$* implib

$(TEST_SEH:%=%_clean): %_clean:
	$(MAKE) -C seh/$* clean

$(TEST_SEH:%=%_dist): %_dist:
	$(MAKE) -C seh/$* dist

$(TEST_SEH:%=%_install): %_install:
	$(MAKE) -C seh/$* install

.PHONY: $(TEST_SEH) $(TEST_SEH:%=%_implib) $(TEST_SEH:%=%_clean) $(TEST_SEH:%=%_install) $(TEST_SEH:%=%_dist)


#
# Regression Test Applications
#
$(TEST_REGRESSIONS): %:
	$(MAKE) -C regres/$*

$(TEST_REGRESSIONS:%=%_implib): %_implib:
	$(MAKE) -C regres/$* implib

$(TEST_REGRESSIONS:%=%_clean): %_clean:
	$(MAKE) -C regres/$* clean

$(TEST_REGRESSIONS:%=%_dist): %_dist:
	$(MAKE) -C regres/$* dist

$(TEST_REGRESSIONS:%=%_install): %_install:
	$(MAKE) -C regres/$* install

.PHONY: $(TEST_REGRESSIONS) $(TEST_REGRESSIONS:%=%_implib) $(TEST_REGRESSIONS:%=%_clean) $(TEST_REGRESSIONS:%=%_install) $(TEST_REGRESSIONS:%=%_dist)


etags:
	find . -name "*.[ch]" -print | etags --language=c -

# EOF

