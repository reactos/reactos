# Makefile

CPP = g++.exe
CC  = gcc.exe
WINDRES = windres.exe

DIR_OBJECTS = obj/
DIR_INCLUDE =

DEFS     = -DINCLUDE_PICTUREBOX -DINCLUDE_MULTI_WINDOWS
DEFS    += -DINCLUDE_WINDOWOPTIONS -DINCLUDE_MAXIMIZE
DEFS    += -DINCLUDE_MINIMIZE -DINCLUDE_RESTORE
DEFS    += -DINCLUDE_EXTENDEDSELECTIONS
DEFS    += -D_FDEDIT_PRIVATE_CLIPBOARD
INCLUDE  = 
DEBUG    = -Wall #-W -ansi #-Xlinker -M # -Werror
OPTIMIZE = -O2 -fomit-frame-pointer -mpreferred-stack-boundary=2
OPTIMIZE+= -march=pentium -mtune=pentium
LIBS     = -mconsole -s

CFLAGS   = $(DEFS) $(INCLUDE) $(DEBUG) $(OPTIMIZE)

FILES  = window   message textbox  listbox  normal   dialogs  menus    rdonly
FILES += config   menu    menubar  popdown  rect     applicat keys     sysmenu
FILES += dialbox  button  fileopen msgbox   helpbox  log      lists    statbar
FILES += combobox pictbox calendar clipbord search   dfalloc  editor   editbox
FILES += checkbox text    radio    box      spinbutt watch    slidebox direct
FILES += decomp   edit

FILES += timer_w32 country_w32 dir_w32 video_w32 console_w32 clipbord_w32

OBJS= $(addprefix $(DIR_OBJECTS), $(addsuffix .o, $(FILES)))

all : edit.exe edit.hlp

edit.exe: $(OBJS)
	$(CC) $^ -o $@ $(LIBS)

$(DIR_OBJECTS)%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

huffc.exe : $(DIR_OBJECTS)huffc.o $(DIR_OBJECTS)htree.o
	$(CC) $^ -o $@ $(LIBS)

fixhelp.exe : $(DIR_OBJECTS)fixhelp.o $(DIR_OBJECTS)decomp.o
	$(CC) $^ -o $@ $(LIBS)

edit.hlp : edit.txt huffc.exe fixhelp.exe
	huffc edit.txt $@

