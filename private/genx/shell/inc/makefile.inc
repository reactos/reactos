#
#
#  R E A D   T H I S
#
#  See the inference rules below (the section marked "Inference rules") to
#  see if any of them cover the rule that you want performed for your file.
#  The inference rules should cover most of the cases.  If they don't, try
#  to add a new inference rule rather than adding a special-case rule for
#  your file.
#

# Add intermediate build files to WORK_FILES so they can be cleaned up.
WORK_FILES = \
    $O\cmnquery.x   \
    $O\cmnquryp.x   \
    $O\comctrlp.x   \
    $O\commctrl.x   \
    $O\commdlg.x    \
    $O\commdlgp.x   \
    $O\dsclient.x   \
    $O\dsclintp.x   \
    $O\dsquery.x    \
    $O\dsqueryp.x   \
    $O\multimon.x   \
    $O\multimop.x   \
    $O\prsht.x      \
    $O\prshtp.x     \
    $O\shellapi.x   \
    $O\shlapip.x    \
    $O\shlguid.x    \
    $O\shlguidp.x   \
    $O\shlobj.x     \
    $O\shlobjp.x    \
    $O\shlwapi.x    \
    $O\shlwapip.x   \
    shldisp.idl     \
    shldispp.idl    \


# This cleans up everything in these macro lists.  The magic for() command below
# cleans up any .h and .tlb files generated from the .idl files.
clean:
    -del /F /Q $(NTTARGETFILE0)
    -del /F /Q $(NTTARGETFILES)
    -del /F /Q $(RELOCATED_FILES)
    -del /F /Q $(WORK_FILES)
    -for %i in (*.idl) do del %~ni*.tlb 2>nul & del $(_OBJ_DIR)\%~ni*.h 2>nul

#
# Special rules for splitting out .w files
#

# shellapi.h and shlapip.h

$O\shellapi.x $O\shlapip.x: shellapi.w
    hsplit -e -o $O\shellapi.x $O\shlapip.x $?

$(SDK_INC_PATH)\shellapi.h: $O\shellapi.x
    wcshdr < $? > $@

$(PRIVATE_INC_PATH)\shlapip.h: $O\shlapip.x
    wcshdr < $? > $@


# commctrl.h and comctrlp.h

$O\commctrl.x $O\comctrlp.x: commctrl.w
    hsplit -e -o $O\commctrl.x $O\comctrlp.x $?

$(SDK_INC_PATH)\commctrl.h: $O\commctrl.x
    wcshdr < $? > $@
    type << > $(SDK_INC_PATH)\commctrl.rh
/*++ BUILD Version: ????     Increment this if a change has global effects

$(COPYRIGHT_STRING)

Module Name:

    commctrl.rh

Abstract:

    This module defines the 32-Bit Windows resource codes from commctrl.h.

Revision History:

--*/
<<
    hextract -o $(SDK_INC_PATH)\commctrl.rh -lt r_commctrl -bt begin_r_commctrl end_r_commctrl $(SDK_INC_PATH)\commctrl.h

$(PRIVATE_INC_PATH)\comctrlp.h: $O\comctrlp.x
    wcshdr < $? > $@


# prsht.h and prshtp.h

$O\prsht.x $O\prshtp.x: prsht.w
    hsplit -e -o $O\prsht.x $O\prshtp.x $?

$(SDK_INC_PATH)\prsht.h: $O\prsht.x
    wcshdr < $? > $@

$(PRIVATE_INC_PATH)\prshtp.h: $O\prshtp.x
    wcshdr < $? > $@


# commdlg.h and commdlgp.h

$O\commdlg.x $O\commdlgp.x: commdlg.w
    hsplit -e -o $O\commdlg.x $O\commdlgp.x $?

$(SDK_INC_PATH)\commdlg.h: $O\commdlg.x
    wcshdr < $? > $@

$(PRIVATE_INC_PATH)\commdlgp.h: $O\commdlgp.x
    wcshdr < $? > $@


# shlobj.h shlobjp.h

$O\shlobj.x $O\shlobjp.x: shlobj.w
    hsplit -e -o $O\shlobj.x $O\shlobjp.x $?

$(SDK_INC_PATH)\shlobj.h: $O\shlobj.x
    wcshdr < $? > $@

$(PRIVATE_INC_PATH)\shlobjp.h: $O\shlobjp.x
    wcshdr < $? > $@


# shlwapi.h and shlwapip.h

$O\shlwapi.x $O\shlwapip.x: shlwapi.w
    hsplit -e -o $O\shlwapi.x $O\shlwapip.x $?

$(SDK_INC_PATH)\shlwapi.h: $O\shlwapi.x
    wcshdr < $? > $@

$(PRIVATE_INC_PATH)\shlwapip.h: $O\shlwapip.x
    wcshdr < $? > $@



# shldisp.idl and shldispp.idl
shldisp.idl shldispp.idl shldisp.tlb shldispp.tlb $(_OBJ_DIR)\shldisp.h $(_OBJ_DIR)\shldispp.h: shldisp.idlw

$(SDK_INC_PATH)\shldisp.idl: shldisp.idl

$(BASEDIR)\private\inc\shldispp.idl: shldispp.idl

$(SDK_LIB_PATH)\shldisp.tlb: shldisp.tlb

$(BASEDIR)\private\lib\$(O)\shldispp.tlb: shldispp.tlb

$(SDK_INC_PATH)\shldisp.h: $(_OBJ_DIR)\shldisp.h
     copy $? $@

$(BASEDIR)\private\inc\shldispp.h: $(_OBJ_DIR)\shldispp.h
     copy $? $@

# shlguid.h and shkguidp.h

$O\shlguid.x $O\shlguidp.x: shlguid.w
    hsplit -e -o $O\shlguid.x $O\shlguidp.x $?

$(SDK_INC_PATH)\shlguid.h: $O\shlguid.x
    wcshdr < $? > $@

$(PRIVATE_INC_PATH)\shlguidp.h: $O\shlguidp.x
    wcshdr < $? > $@


# multimon.h and multimop.h

$O\multimon.x $O\multimop.x: multimon.w
    hsplit -e -o $O\multimon.x $O\multimop.x $?

$(SDK_INC_PATH)\multimon.h: $O\multimon.x
    wcshdr < $? > $@

$(PRIVATE_INC_PATH)\multimop.h: $O\multimop.x
    wcshdr < $? > $@


# dsquery.h and dsqueryp.h

$O\dsquery.x $O\dsqueryp.x: dsquery.w
    hsplit -e -o $O\dsquery.x $O\dsqueryp.x $?

$(SDK_INC_PATH)\dsquery.h: $O\dsquery.x
    wcshdr < $? > $@

$(PRIVATE_INC_PATH)\dsqueryp.h: $O\dsqueryp.x
    wcshdr < $? > $@


# dsclient.h and dsclintp.h

$O\dsclient.x $O\dsclintp.x: dsclient.w
    hsplit -e -o $O\dsclient.x $O\dsclintp.x $?

$(SDK_INC_PATH)\dsclient.h: $O\dsclient.x
    wcshdr < $? > $@

$(PRIVATE_INC_PATH)\dsclintp.h: $O\dsclintp.x
    wcshdr < $? > $@

#
# some NT components use dsshell.h / dsshellp.h so lets
# ensure they continue to build.
#

$(SDK_INC_PATH)\dsshell.h: dsclient.w 
    type << > $@
#include "dsclient.h"
<<

$(PRIVATE_INC_PATH)\dsshellp.h: dsclient.w 
    type << > $@
#include "dsclintp.h"
<<

# cmnquery.h and cmnquryp.h

$O\cmnquery.x $O\cmnquryp.x: cmnquery.w
    hsplit -e -o $O\cmnquery.x $O\cmnquryp.x $?

$(SDK_INC_PATH)\cmnquery.h: $O\cmnquery.x
    wcshdr < $? > $@

$(PRIVATE_INC_PATH)\cmnquryp.h: $O\cmnquryp.x
    wcshdr < $? > $@


#
# Non-obvious Dependencies
#

shdisp.odl: .\exdisp.tlb


#
# Inference rules.  These help build whatever is in the NTTARGETFILES and
# NTTARGETFILE0 lists.  Most of these simply copy files.
#

.SUFFIXES:.h .idlw .hpp .idl .tlb .dlg

{}.idlw{}.idl:
    hsplit -e -o $(*R).idl $(*R)p.idl $**

# BUGBUG (scotth): remove this intrinsic once all files have been moved into shell\inc.
# Copy .idl file from .. to sdk\inc
{..}.idl{$(SDK_INC_PATH)}.idl:
    copy $** $@

# Copy .idl file from here to sdk\inc
{}.idl{$(SDK_INC_PATH)}.idl:
    copy $** $@

# Copy .idl file from here to private\inc
{}.idl{$(PRIVATE_INC_PATH)}.idl:
    copy $** $@

# Copy .tlb file from here to sdk\lib\*
{}.tlb{$(SDK_LIB_PATH)}.tlb:
    copy $** $@

# Copy .tlb file from here to private\lib\*
{}.tlb{$(BASEDIR)\private\lib\$(O)}.tlb:
    copy $** $@

# Copy .h file from here to private\inc
{}.h{$(PRIVATE_INC_PATH)}.h:
    copy $** $@

# Copy .h file from here to sdk\inc
{}.h{$(SDK_INC_PATH)}.h:
    copy $** $@

# Copy .h file from obj dir to sdk\inc
{$O}.h{$(SDK_INC_PATH)}.h:
    copy $** $@

# Copy .h file from obj dir (during pass 0) to sdk\inc
{$(PASS0_HEADERDIR)}.h{$(SDK_INC_PATH)}.h:
    copy $** $@

# Copy .dlg file from here to sdk\inc
{}.dlg{$(SDK_INC_PATH)}.dlg:
    copy $** $@

# Copy .hpp file from here to sdk\inc
{}.hpp{$(SDK_INC_PATH)}.hpp:
    copy $** $@


#
# Other special rules
#

