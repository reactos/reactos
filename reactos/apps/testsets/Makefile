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
	make -C $*

$(TEST_SETS:%=%_implib): %_implib:
	make -C $* implib

$(TEST_SETS:%=%_clean): %_clean:
	make -C $* clean

$(TEST_SETS:%=%_dist): %_dist:
	make -C $* dist

$(TEST_SETS:%=%_install): %_install:
	make -C $* install

.PHONY: $(TEST_SETS) $(TEST_SETS:%=%_implib) $(TEST_SETS:%=%_clean) $(TEST_SETS:%=%_install) $(TEST_SETS:%=%_dist)


#
# Kernel32 Test Applications
#
$(TEST_KERNEL32): %:
	make -C kernel32/$*

$(TEST_KERNEL32:%=%_implib): %_implib:
	make -C kernel32/$* implib

$(TEST_KERNEL32:%=%_clean): %_clean:
	make -C kernel32/$* clean

$(TEST_KERNEL32:%=%_dist): %_dist:
	make -C kernel32/$* dist

$(TEST_KERNEL32:%=%_install): %_install:
	make -C kernel32/$* install

.PHONY: $(TEST_KERNEL32) $(TEST_KERNEL32:%=%_implib) $(TEST_KERNEL32:%=%_clean) $(TEST_KERNEL32:%=%_install) $(TEST_KERNEL32:%=%_dist)


#
# msvcrt Test Applications
#
$(TEST_MSVCRT): %:
	make -C msvcrt/$*

$(TEST_MSVCRT:%=%_implib): %_implib:
	make -C msvcrt/$* implib

$(TEST_MSVCRT:%=%_clean): %_clean:
	make -C msvcrt/$* clean

$(TEST_MSVCRT:%=%_dist): %_dist:
	make -C msvcrt/$* dist

$(TEST_MSVCRT:%=%_install): %_install:
	make -C msvcrt/$* install

.PHONY: $(TEST_MSVCRT) $(TEST_MSVCRT:%=%_implib) $(TEST_MSVCRT:%=%_clean) $(TEST_MSVCRT:%=%_install) $(TEST_MSVCRT:%=%_dist)


#
# COM Test Applications
#
$(TEST_COM): %:
	make -C com/$*

$(TEST_COM:%=%_implib): %_implib:
	make -C com/$* implib

$(TEST_COM:%=%_clean): %_clean:
	make -C com/$* clean

$(TEST_COM:%=%_dist): %_dist:
	make -C com/$* dist

$(TEST_COM:%=%_install): %_install:
	make -C com/$* install

.PHONY: $(TEST_COM) $(TEST_COM:%=%_implib) $(TEST_COM:%=%_clean) $(TEST_COM:%=%_install) $(TEST_COM:%=%_dist)


#
# SEH Test Applications
#
$(TEST_SEH): %:
	make -C seh/$*

$(TEST_SEH:%=%_implib): %_implib:
	make -C seh/$* implib

$(TEST_SEH:%=%_clean): %_clean:
	make -C seh/$* clean

$(TEST_SEH:%=%_dist): %_dist:
	make -C seh/$* dist

$(TEST_SEH:%=%_install): %_install:
	make -C seh/$* install

.PHONY: $(TEST_SEH) $(TEST_SEH:%=%_implib) $(TEST_SEH:%=%_clean) $(TEST_SEH:%=%_install) $(TEST_SEH:%=%_dist)


#
# Regression Test Applications
#
$(TEST_REGRESSIONS): %:
	make -C regres/$*

$(TEST_REGRESSIONS:%=%_implib): %_implib:
	make -C regres/$* implib

$(TEST_REGRESSIONS:%=%_clean): %_clean:
	make -C regres/$* clean

$(TEST_REGRESSIONS:%=%_dist): %_dist:
	make -C regres/$* dist

$(TEST_REGRESSIONS:%=%_install): %_install:
	make -C regres/$* install

.PHONY: $(TEST_REGRESSIONS) $(TEST_REGRESSIONS:%=%_implib) $(TEST_REGRESSIONS:%=%_clean) $(TEST_REGRESSIONS:%=%_install) $(TEST_REGRESSIONS:%=%_dist)


etags:
	find . -name "*.[ch]" -print | etags --language=c -

# EOF

