#****************************** File Header ******************************\
# File Name: makefile.inc
#
# Copyright (c) 1985-96, Microsoft Corporation
#
# Inference rules to build generated header files
#
# History:
# Feb-14-1996 GerardoB Created
#***************************************************************************/
cscall.h: ..\cscall.tpl scf.lst
    listmung scf.lst ..\cscall.tpl >cscall.h

callback.h: ..\callback.tpl cb.lst
    listmung cb.lst ..\callback.tpl >callback.h

msgdef.h: ..\msgdef.tpl msgdef.lst
    listmung msgdef.lst ..\msgdef.tpl >msgdef.h

scf.lst: ..\scf.lst
    @echo Creating $@ from $**
    $(C_PREPROCESSOR)  $** > $@

cb.lst: ..\cb.lst
    @echo Creating $@ from $**
    $(C_PREPROCESSOR)  $** > $@

msgdef.lst: ..\msgdef.lst
    @echo Creating $@ from $**
    $(C_PREPROCESSOR)  $** > $@


