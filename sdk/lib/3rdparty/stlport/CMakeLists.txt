
#uncomment this if you want to test c++ compilation
#add_subdirectory(test)

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
    src/stlport_prefix.h)

add_library(stlport
    ${SOURCE}
    src/cxa.c
    src/c_locale.c)

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(stlport PRIVATE -Wno-missing-braces -Wno-unused-local-typedef)
    target_compile_options(stlport PRIVATE -Wno-infinite-recursion -Wno-deprecated-register)
    target_compile_options(stlport PRIVATE -Wno-tautological-unsigned-zero-compare)
endif()

target_include_directories(stlport PRIVATE ${REACTOS_SOURCE_DIR}/sdk/include/c++/stlport)
target_compile_definitions(stlport PRIVATE
    _STLP_USE_EXCEPTIONS _DLL __USE_CRTIMP
    _BUILD_STLPORT NATIVE_CPP_INCLUDE=${REACTOS_SOURCE_DIR}/sdk/include/c++)

target_compile_definitions(stlport PUBLIC
    _CRT_DECLARE_NONSTDC_NAMES=1
    _CRT_NONSTDC_NO_DEPRECATE
)

target_include_directories(stlport INTERFACE "$<$<COMPILE_LANGUAGE:CXX>:${REACTOS_SOURCE_DIR}/sdk/include/c++/stlport>")
target_compile_definitions(stlport INTERFACE "$<$<COMPILE_LANGUAGE:CXX>:NATIVE_CPP_INCLUDE=${REACTOS_SOURCE_DIR}/sdk/include/c++>")
set_target_cpp_properties(stlport WITH_EXCEPTIONS WITH_RTTI)

add_dependencies(stlport xdk)
add_pch(stlport src/stlport_prefix.h SOURCE)
