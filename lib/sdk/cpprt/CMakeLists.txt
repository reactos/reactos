
set_cpp(WITH_EXCEPTIONS)

list(APPEND SOURCE
    ehvec.cpp
    typeinfo.cpp)

add_asm_files(cpprt_asm i386/cpprt.s)
add_library(cpprt ${SOURCE} ${cpprt_asm})
