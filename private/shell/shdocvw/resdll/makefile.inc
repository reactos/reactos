..\error.dls : ..\error.dlg 
    $(C_PREPROCESSOR_NAME) /EP ..\error.dlg > $@

..\error.dlx : ..\error.dls ..\dlg.sed
    sed -f ..\dlg.sed < ..\error.dls > $@

..\about.dls : ..\about.dlg 
    $(C_PREPROCESSOR_NAME) /EP ..\about.dlg > $@

..\about.dlx : ..\about.dls ..\dlg.sed
    sed -f ..\dlg.sed < ..\about.dls > $@

..\findinc.dls : ..\findinc.dlg ..\dialogs.dh
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) /EP ..\findinc.dlg > $@

..\findinc.dlx : ..\findinc.dls ..\findinc.sed
    sed -f ..\findinc.sed < ..\findinc.dls > $@

..\find.dls : ..\find.dlg ..\dialogs.dh ..\findinc.dlg
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) /EP ..\find.dlg > $@

..\find.dlx : ..\find.dls ..\dlg.sed
    sed -f ..\dlg.sed < ..\find.dls > $@

..\bidifind.dls : ..\bidifind.dlg ..\dialogs.dh ..\findinc.dlg
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) /EP ..\bidifind.dlg > $@

..\bidifind.dlx : ..\bidifind.dls ..\dlg.sed
    sed -f ..\dlg.sed < ..\bidifind.dls > $@

..\orgfav.dls : ..\orgfav.dlg 
    $(C_PREPROCESSOR_NAME) /EP ..\orgfav.dlg > $@

..\ieerror.dls : ..\ieerror.dlg 
    $(C_PREPROCESSOR_NAME) /EP ..\ieerror.dlg > $@

..\ieerror.dlx : ..\ieerror.dls ..\dlg.sed
    sed -f ..\dlg.sed < ..\ieerror.dls > $@

..\anchbrws.pps : ..\anchbrws.ppg
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) /EP ..\anchbrws.ppg > $@

..\anchbrws.ppx: ..\anchbrws.pps
    sed -f ..\dlg.sed < ..\anchbrws.pps > $@

..\docbrows.pps: ..\docbrows.ppg
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) /EP ..\docbrows.ppg > $@

..\docbrows.ppx : ..\docbrows.pps
    sed -f ..\dlg.sed < ..\docbrows.pps > $@

..\imgbrows.pps : ..\imgbrows.ppg
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) /EP ..\imgbrows.ppg > $@

..\imgbrows.ppx : ..\imgbrows.pps
    sed -f ..\dlg.sed < ..\imgbrows.pps > $@

..\analyze.dls : ..\analyze.dlg ..\dialogs.dh
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) /EP ..\analyze.dlg > $@

..\analyze.dlx : ..\analyze.dls ..\dlg.sed
    sed -f ..\dlg.sed < ..\analyze.dls > $@

SHDOCLCR_DEP = \
    ..\error.dlx \
    ..\about.dlx \
    ..\findinc.dlx \
    ..\find.dlx \
    ..\bidifind.dlx \
    ..\orgfav.dls \
    ..\ieerror.dlx \
    ..\anchbrws.ppx \
    ..\docbrows.ppx \
    ..\imgbrows.ppx \
    ..\analyze.dlx \


..\shdoclc.rc : $(SHDOCLCR_DEP)

!include $(CCSHELL_DIR)\makefile.inc
