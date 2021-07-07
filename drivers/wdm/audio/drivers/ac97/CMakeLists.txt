
set_cpp()

list(APPEND SOURCE
    adapter.cpp
    common.cpp
    mintopo.cpp
    prophnd.cpp
    rtminiport.cpp
    rtstream.cpp
    wavepciminiport.cpp
    wavepcistream.cpp)

add_library(ac97 MODULE
    ${SOURCE}
    ac97.rc)

target_link_libraries(ac97 stdunk libcntpr uuid)
set_module_type(ac97 wdmdriver UNICODE)
add_importlibs(ac97 portcls hal ntoskrnl)

if(USE_CLANG_CL OR (NOT MSVC))
    target_compile_options(ac97 PRIVATE -Wno-write-strings -Wno-switch)
endif()

add_cd_file(TARGET ac97 DESTINATION reactos/system32/drivers FOR all)
add_cd_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/ac97.inf" DESTINATION reactos/inf FOR all)
