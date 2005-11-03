# Important
.EXPORT_ALL_VARIABLES:

# Windows is default host environment
ifeq ($(HOST),)
HOST = mingw32-windows
endif

#
# Choose various options
#
ifeq ($(HOST),mingw32-linux)
PREFIX=
EXE_POSTFIX :=
EXE_PREFIX := ./
DOSCLI =
SEP := /
endif

ifeq ($(HOST),mingw32-windows)
PREFIX =
EXE_PREFIX :=
EXE_POSTFIX := .exe
DOSCLI = yes
SEP := \$(EMPTY_VAR)
endif

CC = $(PREFIX)gcc
HOST_CC = gcc
TOOLS_PATH = $(PATH_TO_TOP)/tools
CP = $(TOOLS_PATH)/rcopy
RM = $(TOOLS_PATH)/rdel
RMDIR = $(TOOLS_PATH)/rrmdir
RMKDIR = $(TOOLS_PATH)/rmkdir

