WINEBUILD_BASE = $(TOOLS_BASE)$(SEP)winebuild

WINEBUILD_BASE_DIR = $(INTERMEDIATE)$(WINEBUILD_BASE)
WINEBUILD_BASE_DIR_EXISTS = $(WINEBUILD_BASE_DIR)$(SEP)$(EXISTS)

$(WINEBUILD_BASE_DIR_EXISTS): $(TOOLS_BASE_DIR_EXISTS)
	$(ECHO_MKDIR)
	${mkdir} $(WINEBUILD_BASE_DIR)
	@echo . > $@

WINEBUILD_TARGET = \
	$(WINEBUILD_BASE_DIR)$(SEP)winebuild$(EXEPOSTFIX)

WINEBUILD_SOURCES = $(addprefix $(WINEBUILD_BASE)$(SEP), \
	import.c \
	main.c \
	parser.c \
	res16.c \
	res32.c \
	spec32.c \
	utils.c \
	mkstemps.c \
	)

WINEBUILD_OBJECTS = \
  $(addprefix $(INTERMEDIATE), $(WINEBUILD_SOURCES:.c=.o))

WINEBUILD_HOST_CFLAGS = -D__USE_W32API -Iinclude/wine

WINEBUILD_HOST_LFLAGS = -g

.PHONY: winebuild
winebuild: $(WINEBUILD_TARGET)

$(WINEBUILD_TARGET): $(WINEBUILD_OBJECTS)
	$(ECHO_LD)
	${host_gcc} $(WINEBUILD_OBJECTS) $(WINEBUILD_HOST_LFLAGS) -o $@

$(WINEBUILD_BASE_DIR)$(SEP)import.o: $(WINEBUILD_BASE)$(SEP)import.c $(WINEBUILD_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WINEBUILD_HOST_CFLAGS) -c $< -o $@

$(WINEBUILD_BASE_DIR)$(SEP)main.o: $(WINEBUILD_BASE)$(SEP)main.c $(WINEBUILD_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WINEBUILD_HOST_CFLAGS) -c $< -o $@

$(WINEBUILD_BASE_DIR)$(SEP)parser.o: $(WINEBUILD_BASE)$(SEP)parser.c $(WINEBUILD_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WINEBUILD_HOST_CFLAGS) -c $< -o $@

$(WINEBUILD_BASE_DIR)$(SEP)res16.o: $(WINEBUILD_BASE)$(SEP)res16.c $(WINEBUILD_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WINEBUILD_HOST_CFLAGS) -c $< -o $@

$(WINEBUILD_BASE_DIR)$(SEP)res32.o: $(WINEBUILD_BASE)$(SEP)res32.c $(WINEBUILD_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WINEBUILD_HOST_CFLAGS) -c $< -o $@

$(WINEBUILD_BASE_DIR)$(SEP)spec32.o: $(WINEBUILD_BASE)$(SEP)spec32.c $(WINEBUILD_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WINEBUILD_HOST_CFLAGS) -c $< -o $@

$(WINEBUILD_BASE_DIR)$(SEP)utils.o: $(WINEBUILD_BASE)$(SEP)utils.c $(WINEBUILD_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WINEBUILD_HOST_CFLAGS) -c $< -o $@

$(WINEBUILD_BASE_DIR)$(SEP)mkstemps.o: $(WINEBUILD_BASE)$(SEP)mkstemps.c $(WINEBUILD_BASE_DIR_EXISTS)
	$(ECHO_CC)
	${host_gcc} $(WINEBUILD_HOST_CFLAGS) -c $< -o $@

.PHONY: winebuild_clean
winebuild_clean:
	-@$(rm) $(WINEBUILD_TARGET) $(WINEBUILD_OBJECTS) 2>$(NUL)
clean: winebuild_clean
