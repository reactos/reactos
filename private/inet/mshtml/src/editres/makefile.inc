# Run the HTML dialogs though the preprocessor to remove comments
# then through a sed script to reomave extra whitespace.

$O\forchar.dls : forchar.dlg dialogs.dh
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) /EP forchar.dlg > $@

$O\forchar.dlx : $O\forchar.dls dlg.sed
    sed -f dlg.sed < $O\forchar.dls > $@

$O\insimage.dls : insimage.dlg dialogs.dh
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) /EP insimage.dlg > $@

$O\insimage.dlx : $O\insimage.dls dlg.sed
    sed -f dlg.sed < $O\insimage.dls > $@ 

$O\edlink.dls : edlink.dlg dialogs.dh
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) /EP edlink.dlg > $@

$O\edlink.dlx : $O\edlink.dls dlg.sed
    sed -f dlg.sed < $O\edlink.dls > $@

$O\edbook.dls : edbook.dlg dialogs.dh
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) /EP edlink.dlg > $@

$O\edbook.dlx : $O\edbook.dls dlg.sed
    sed -f dlg.sed < $O\edbook.dls > $@          

# Run the HTML property pages through a sed
# script to remove extra whitespaces.

MSHTMLER_DEP = \
    $O\forchar.dlx \
    $O\insimage.dlx \
    $O\edlink.dlx 

mshtmler.rc : $(MSHTMLER_DEP)
