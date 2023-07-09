$(OBJ_CRT_PDB) : $(MAIN_CRT_PDB)
    copy $** $@

$O\liblist.out : $(MAIN_CRT_LIB) makefile.inc
    @link -lib -nologo -list $(MAIN_CRT_LIB) > $O\liblist.out

$O\abs.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\adj_fdiv.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\bridge.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\bsearch.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\87disp.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\87except.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\87tran.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\captcont.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\chandler.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\chkstk.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\common.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\constpow.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\divdat.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\dpml_exc.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\ehprolog.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\ehunwind.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\ehvecctr.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\ehvecdtr.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\ehveccvb.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\exsup.obj    : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\exsup3.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\extzv.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\frame.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\fpctrl.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\fpexcept.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\frnd.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\ftol.obj     : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\genexcep.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\ghandler.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\hooks.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\ieeemisc.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\insv.obj     : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\jmpuwind.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\ldexp.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\llmul.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\llshl.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\lfind.obj    : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\lowhelpr.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\matherr.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\memchr.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\memcmp.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\memmove.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\mlock.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\otsdiv.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\pow.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\pow_tabt.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\powcerr.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\powf.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\powhlp.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\pows.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\qsort.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\rtti.obj     : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\scmpeql.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\sfill.obj    : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\smove.obj    : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\smovem.obj    : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\stdexcpt.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\strcat.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\strcmp_.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\strcpy_.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\strlen_.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\strpbrk.obj   : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\strrchr.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\szero.obj    : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\tenpow.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\throw.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\tidtable.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\trnsctrl.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\typname.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\ulldiv.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\undname.obj  : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\unhandld.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\user.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\util.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\validate.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\winxfltr.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\xcptmisc.obj : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\memsets.obj    : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\memcpys.obj    : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2

$O\rotr.obj    : $(MAIN_CRT_LIB) makefile.inc $O\liblist.out
    @qgrep \\$(@B).obj $O\liblist.out > $O\tmp.1 & \
       if errorlevel 1 qgrep -B $(@B).obj $O\liblist.out > $O\tmp.1
    @cat extract.txt $O\tmp.1 > $O\tmp.2
    lib -out:$@ $(MAIN_CRT_LIB) @$O\tmp.2
