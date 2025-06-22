remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

include_directories(../../include)
include_directories(../../include/private)
include_directories(../../include/vulkan)
add_library(vkd3d        cache.c
                         command.c
                         device.c
                         resource.c
                         state.c
                         utils.c
                         vkd3d_main.c)
add_dependencies(vkd3d psdk d3d_idl_headers)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(vkd3d PRIVATE -Wno-implicit-function-declaration -Wno-format -Wno-write-strings -Wno-unknown-pragmas -Wno-switch -Wno-format-extra-args -Wno-format-overflow)
endif()

if(MSVC)
    target_compile_options(vkd3d PRIVATE /wd4146 /wd4090 /wd4267 /wd4309 /wd4305 /wd4715 /wd4098)
endif()
