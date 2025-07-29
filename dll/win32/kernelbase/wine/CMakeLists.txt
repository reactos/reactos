add_definitions(-D__WINESRC__ -D_KERNEL32_ -DWINVER=0x601 -D_WIN32_WINNT=0x601)
add_definitions(
    -D__WINESRC__
    -D_WINE
    -D__ROS_LONG64__
    -Dwcsnicmp=_wcsnicmp
    -Dwcsicmp=_wcsicmp
    -Dstrnicmp=_strnicmp
    -Dswprintf=_swprintf
    -D_KERNELBASE_)

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/wine)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR
   CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # Silence GCC/Clang warnings
    add_compile_options(
        -Wno-unused-function
        -Wno-unknown-pragmas
        -Wno-unused-variable
        -Wno-unused-value
        -Wno-unused-function
    )
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-reorder>)
endif()

#list(APPEND SOURCE_PATH
#    path.c
#    )
#
#add_library(kernelbase_path ${SOURCE_PATH})
#target_link_libraries(kernelbase_path ${PSEH_LIB})
#add_dependencies(kernelbase_path psdk)

if(NOT MSVC)
    target_compile_options(kernelbase_path PRIVATE -Wno-unused-variable)
    target_compile_options(kernelbase_path PRIVATE -Wno-unused-function)
endif()

list(APPEND SOURCE_STATIC
    #console.c #looks like ReactOS console.c is nothing like wine.
    )

add_library(kernelbase_static ${SOURCE_STATIC})
target_link_libraries(kernelbase_static psapi_static ${PSEH_LIB})
add_dependencies(kernelbase_static psdk)

if(NOT MSVC)
    target_compile_options(kernelbase_static PRIVATE -Wno-unused-variable)
    target_compile_options(kernelbase_static PRIVATE -Wno-unused-function)
endif()

