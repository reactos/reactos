
#uncomment this if you want to test c++ compilation
#add_subdirectory(test)

add_definitions(
    -D_STLP_USE_EXCEPTIONS
    -D_DLL -D__USE_CRTIMP
    -D_BUILD_STLPORT)

set_cpp()

list(APPEND SOURCE
    src/allocators.cpp
    src/bitset.cpp
    src/codecvt.cpp
    src/collate.cpp
    src/complex.cpp
    src/complex_io.cpp
    src/complex_trig.cpp
    src/ctype.cpp
    src/dll_main.cpp
    src/facets_byname.cpp
    src/fstream.cpp
    src/ios.cpp
    src/iostream.cpp
    src/istream.cpp
    src/locale.cpp
    src/locale_catalog.cpp
    src/locale_impl.cpp
    src/messages.cpp
    src/monetary.cpp
    src/num_get.cpp
    src/num_get_float.cpp
    src/num_put.cpp
    src/num_put_float.cpp
    src/numpunct.cpp
    src/ostream.cpp
    src/sstream.cpp
    src/stdio_streambuf.cpp
    src/string.cpp
    src/strstream.cpp
    src/time_facets.cpp
    src/c_locale.c
    src/cxa.c)

add_library(stlport ${SOURCE})
