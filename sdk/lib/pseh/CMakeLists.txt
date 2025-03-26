
if(ARCH STREQUAL "i386")
    list(APPEND ASM_SOURCE
        i386/seh.s
        i386/seh_prolog.s)
elseif(ARCH STREQUAL "amd64")
    list(APPEND ASM_SOURCE
        amd64/seh.s
        amd64/seh_prolog.s)
elseif(ARCH STREQUAL "arm")
    list(APPEND ASM_SOURCE
        arm/seh_prolog.s)
endif()

if(MSVC OR ((CMAKE_C_COMPILER_ID STREQUAL "Clang") AND (ARCH STREQUAL "amd64")))
    list(APPEND SOURCE dummy.c)
    add_asm_files(pseh_asm ${ASM_SOURCE})
    add_library(pseh ${SOURCE} ${pseh_asm})
    add_dependencies(pseh asm)
elseif((CMAKE_C_COMPILER_ID STREQUAL "GNU") AND (ARCH STREQUAL "amd64"))
    # for GCC amd64 this is just an interface library, with our home-made plugin
    add_library(pseh INTERFACE)
    if (CMAKE_HOST_WIN32)
        target_compile_options(pseh INTERFACE
            $<$<COMPILE_LANGUAGE:C>:-fplugin=$<TARGET_FILE:native-gcc_plugin_seh>>
            $<$<COMPILE_LANGUAGE:CXX>:-fplugin=$<TARGET_FILE:native-g++_plugin_seh>>)
    else()
        target_compile_options(pseh INTERFACE $<$<COMPILE_LANGUAGE:C,CXX>:-fplugin=$<TARGET_FILE:native-gcc_plugin_seh>>)
    endif()
else()

    if(USE_PSEH3)
        include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/pseh)
        list(APPEND SOURCE
            i386/pseh3.c
            i386/pseh3_i386.S)
    elseif(USE_DUMMY_PSEH)
        list(APPEND SOURCE dummy.c)
    elseif(ARCH STREQUAL "i386")
        list(APPEND SOURCE
            i386/framebased.S
            i386/framebased-gcchack.c
            i386/framebased-gcchack-asm.S)
    elseif(ARCH STREQUAL "amd64")
        list(APPEND SOURCE amd64/framebased.S)
    endif()

    add_library(pseh ${SOURCE} ${ASM_SOURCE})
    target_link_libraries(pseh chkstk)
    add_dependencies(pseh psdk)

    target_include_directories(pseh PRIVATE include/pseh)
endif()

target_include_directories(pseh INTERFACE include)

# Make it clear that we are using PSEH2
if ((CMAKE_C_COMPILER_ID STREQUAL "GNU") OR
    ((CMAKE_C_COMPILER_ID STREQUAL "Clang") AND (NOT (ARCH STREQUAL "amd64"))))
endif()
