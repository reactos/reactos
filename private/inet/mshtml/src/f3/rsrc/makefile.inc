# Run the HTML dialogs though the preprocessor to remove comments
# then through a sed script to reomave extra whitespace.

$O\replace.dls : replace.dlg dialogs.dh
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) /EP replace.dlg > $@

$O\replace.dlx : $O\replace.dls dlg.sed
    sed -f dlg.sed < $O\replace.dls > $@

$O\forpar.dls : forpar.dlg dialogs.dh
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) /EP forpar.dlg > $@

$O\forpar.dlx : $O\forpar.dls dlg.sed
    sed -f dlg.sed < $O\forpar.dls > $@

$O\gobook.dls : gobook.dlg dialogs.dh
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) /EP gobook.dlg > $@

$O\gobook.dlx : $O\gobook.dls dlg.sed
    sed -f dlg.sed < $O\gobook.dls > $@

# Run the HTML property pages through a sed
# script to remove extra whitespaces.

$O\selfreg_mshtml.inf : selfreg.inx
	cleaninf.exe -w selfreg.inx $@

FORMS3R_DEP = \
    $(BASEDIR)\public\sdk\inc\ieverp.h \
    $O\replace.dlx \
    $O\forpar.dlx \
    $O\gobook.dlx \
    backgrnd.ppg \
    backgrnd.jpg \
    clouds.gif \
    waves.gif \
    authfla.gif \
    cforms.ico \
    selbar.cur \
    west.cur \
    south.cur \
    north.cur \
    nomove2d.cur \
    east.cur \
    sw.cur \
    se.cur \
    nw.cur \
    ne.cur \
    hyperlnk.cur \
    hlnocach.cur \
    csimtool.bmp \
    $O\selfreg_mshtml.inf

forms3r.rc : $(FORMS3R_DEP)
