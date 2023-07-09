all: fontreg.exe  fontreg.sym


#-------------------  Abbreviations and Inference Rules  ----------------------

CC = cl  -c -O1 -W3 -D"WIN32" -D"_X86_" -YX -Gf -Zl

!ifdef DEBUG
CC = $(CC) -DDEBUG
!endif

!ifdef LIST
CC = $(CC) -Fc
!endif


{..}.c.obj:
    $(CC) ..\$*.c


{..}.rc.res:
    rc -r  -fo$*.res  ..\$*.rc


#-----------------------------  Dependencies  ---------------------------------


fontreg.obj:  ..\fontreg.c   ..\fontreg.mk  ..\ttf.h


fontreg.res:  ..\fontreg.rc



fontreg.exe:   fontreg.obj  fontreg.res  ..\fontreg.mk
 link @<<
-subsystem:windows
-map:fontreg.map
-entry:WinMain
fontreg.obj
kernel32.lib  gdi32.lib  user32.lib  advapi32.lib
fontreg.res
<<


fontreg.sym:  fontreg.exe
  mapsym fontreg.map


