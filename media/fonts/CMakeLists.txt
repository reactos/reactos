
list(APPEND FONT_FILES
    arial.ttf
    arialbd.ttf
    arialbi.ttf
    ariali.ttf
    DejaVuSansMono-Bold.ttf
    DejaVuSansMono-BoldOblique.ttf
    DejaVuSansMono-Oblique.ttf
    DejaVuSansMono.ttf
    FreeMono.ttf
    FreeMonoBold.ttf
    FreeMonoBoldOblique.ttf
    FreeMonoOblique.ttf
    FreeSans.ttf
    FreeSansBold.ttf
    FreeSansBoldOblique.ttf
    FreeSansOblique.ttf
    FSEX301.ttf
    FSEX301-L2.ttf
    georgia.ttf
    georgiab.ttf
    georgiai.ttf
    georgiaz.ttf
    LibreFranklin-Bold.ttf
    LibreFranklin-BoldItalic.ttf
    LibreFranklin-Italic.ttf
    LibreFranklin-Regular.ttf
    Marlett.ttf
    pala.ttf
    palab.ttf
    palabi.ttf
    palai.ttf
    symbol.ttf
    tahoma.ttf
    tahomabd.ttf
    times.ttf
    timesbd.ttf
    timesbi.ttf
    timesi.ttf
    trebuc.ttf
    trebucbd.ttf
    trebucbi.ttf
    trebucit.ttf
    UniVGA16.ttf)

foreach(item ${FONT_FILES})
    add_cd_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/${item}" DESTINATION reactos/Fonts FOR all)
endforeach(item)
