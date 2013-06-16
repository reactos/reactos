
set_cpp()

add_library(cmicpl SHARED
    cmicpl.cpp
    cmicpl.rc
)

set_module_type(cmicpl cpl UNICODE)

add_importlibs(cmicpl
    shell32
    msvcrt
    kernel32
)

if (NOT MSVC)
  target_link_libraries(cmicpl -lgcc)
endif()

add_cd_file(TARGET cmicpl DESTINATION reactos/system32/drivers FOR all)
