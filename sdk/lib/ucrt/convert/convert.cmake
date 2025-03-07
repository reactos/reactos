
list(APPEND UCRT_CONVERT_SOURCES
    convert/atof.cpp
    convert/atoldbl.cpp
    convert/atox.cpp
    convert/c16rtomb.cpp
    convert/c32rtomb.cpp
    convert/cfout.cpp
    convert/common_utf8.cpp
    convert/cvt.cpp
    convert/fcvt.cpp
    convert/fp_flags.cpp
    convert/gcvt.cpp
    convert/isctype.cpp
    convert/ismbstr.cpp
    convert/iswctype.cpp
    convert/mblen.cpp
    convert/mbrtoc16.cpp
    convert/mbrtoc32.cpp
    convert/mbrtowc.cpp
    convert/mbstowcs.cpp
    convert/mbtowc.cpp
    convert/strtod.cpp
    convert/strtox.cpp
    convert/swab.cpp
    convert/tolower_toupper.cpp
    convert/towlower.cpp
    convert/towupper.cpp
    convert/wcrtomb.cpp
    convert/wcstombs.cpp
    convert/wctomb.cpp
    convert/wctrans.cpp
    convert/wctype.cpp
    convert/xtoa.cpp
    convert/_ctype.cpp
    convert/_fptostr.cpp
    convert/_mbslen.cpp
    convert/_wctype.cpp
)

# All multibyte string functions require the _MBCS macro to be defined
set_source_files_properties(convert/ismbstr.cpp PROPERTIES COMPILE_DEFINITIONS _MBCS)
