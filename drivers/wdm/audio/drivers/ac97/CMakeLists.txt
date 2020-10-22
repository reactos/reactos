
set_cpp()

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
    wavecyclicstream.cpp
)

add_library(ac97smpl MODULE
    ${SOURCE}
    ac97smpl.rc)

target_link_libraries(ac97smpl stdunk libcntpr uuid)
set_module_type(ac97smpl wdmdriver UNICODE)
add_importlibs(ac97smpl portcls hal ntoskrnl)

if(USE_CLANG_CL OR (NOT MSVC))
    target_compile_options(ac97smpl PRIVATE -Wno-write-strings -Wno-switch)
endif()

add_cd_file(TARGET ac97smpl DESTINATION reactos/system32/drivers FOR all)
add_cd_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/ac97smpl.inf" DESTINATION reactos/inf FOR all)
