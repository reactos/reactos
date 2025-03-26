
list(APPEND SOURCE
    adapter.cpp
    common.cpp
    mintopo.cpp
    prophnd.cpp
    miniport.cpp
    stream.cpp
    stream2.cpp
    rtminiport.cpp
    rtstream.cpp
    wavepciminiport.cpp
    wavepcistream.cpp
    wavecyclicminiport.cpp
    wavecyclicstream.cpp)

add_library(ac97 MODULE
    ${SOURCE}
    ac97.rc)

target_link_libraries(ac97 stdunk libcntpr uuid)
set_module_type(ac97 wdmdriver)
add_importlibs(ac97 portcls hal ntoskrnl)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(ac97 PRIVATE -Wno-write-strings -Wno-switch)
endif()

add_cd_file(TARGET ac97 DESTINATION reactos/system32/drivers FOR all)
add_driver_inf(ac97 ac97.inf)
