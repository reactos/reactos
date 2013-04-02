
set_cpp()

add_library(stdunk STATIC cunknown.cpp)

if(MSVC)
    add_target_compile_flags(stdunk "/GR-")
else()
    add_target_compile_flags(stdunk "-fno-exceptions -fno-rtti")
endif()
