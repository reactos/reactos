list(APPEND FONT_FILES
    DejaVuSans.ttf
    DejaVuSans-Bold.ttf
    DejaVuSans-BoldOblique.ttf
    DejaVuSans-Oblique.ttf
    DejaVuSansMono.ttf
    DejaVuSansMono-Bold.ttf
    DejaVuSansMono-BoldOblique.ttf
    DejaVuSansMono-Oblique.ttf
    DejaVuSerif.ttf
    DejaVuSerif-Bold.ttf
    DejaVuSerif-BoldItalic.ttf
    DejaVuSerif-Italic.ttf
    FreeMono.ttf
    FreeMonoBold.ttf
    FreeMonoBoldOblique.ttf
    FreeMonoOblique.ttf
    LiberationMono-Bold.ttf
    LiberationMono-BoldItalic.ttf
    LiberationMono-Italic.ttf
    LiberationMono-Regular.ttf
    LiberationSans-Bold.ttf
    LiberationSans-BoldItalic.ttf
    LiberationSans-Italic.ttf
    LiberationSans-Regular.ttf
    LiberationSerif-Bold.ttf
    LiberationSerif-BoldItalic.ttf
    LiberationSerif-Italic.ttf
    LiberationSerif-Regular.ttf
    Marlett.ttf
    symbol.ttf
    tahoma.ttf
    tahomabd.ttf
)

foreach(item ${FONT_FILES})
    add_cd_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/${item}" DESTINATION reactos/Fonts FOR all)
endforeach(item)