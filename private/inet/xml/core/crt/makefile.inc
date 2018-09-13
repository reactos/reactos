#
# 
# Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
# EXEMPT: copyright change only, no build required
# 
#
$(OBJ_CRT_PDB) : $(MAIN_CRT_PDB)
    copy $** $@

$O\liblist.out : $(MAIN_CRT_LIB)
    @link -lib -nologo -list $(MAIN_CRT_LIB) > $O\liblist.out

$O\bridge.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\bsearch.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\chandler.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\chkstk.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\divdat.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\exsup.obj    : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\exsup3.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\extv.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\extvvol.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\extzv.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\extzvvol.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\frame.obj    : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\ftol.obj     : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\ghandler.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\insv.obj     : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\jmpuwind.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\lfind.obj    : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\mbstowcs.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\mbtowenv.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\memcmp.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\memcpyp.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\memmove.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\memorym.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\otsdiv.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\qsort.obj    : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\rtti.obj     : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\setlocal.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\scmpeql.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\sfill.obj    : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\smove.obj    : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\smovem.obj    : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\stdexcpt.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\strcat.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\strcatm.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\strchr.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\strchrm.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\strchrp.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\strcmp_.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\strcmpm.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\strcmps.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\strcpy_.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\strcpym.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\strlen_.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\strlens.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\strlenm.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\strncmp.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\strrchr.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\strrchrm.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\strrchrp.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\szero.obj    : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\trnsctrl.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\typinfo.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\typname.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\undname.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\unhandld.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\wcscat.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\wcschr.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\wcscmp.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\wcslen.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\wcsncpy.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\wcspbrk.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\wcsrchr.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\wcsrev.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\wcsspn.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\wcsstr.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\wtox.obj     : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\xtoa.obj     : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\xtow.obj     : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\memcpys.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\strcmp.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\strlen.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2
