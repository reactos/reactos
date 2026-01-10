
function(add_bootvid _target _configfile)
    cmake_parse_arguments(_bviddata "" "NAME_ON_CD" "SOURCES;DEFINES" ${ARGN}) ## TODO: Remove SOURCES and DEFINES?

    # Handle the spec file for the dll name
    spec2def(${_target}.dll bootvid.spec ADD_IMPORTLIB)

    # Set default values for the version resource file
    set(REACTOS_STR_INTERNAL_NAME "$<TARGET_NAME:${_target}>")
    set(REACTOS_STR_ORIGINAL_FILENAME "$<TARGET_FILE_NAME:${_target}>")

    # Include the per-target definitions (export SOURCE and COMPILE_DEFINITIONS)
    include(${_configfile})
    list(APPEND SOURCE ${_bviddata_SOURCES})

    # Add the common sources
    list(APPEND SOURCE common.c console.c fontdata.c precomp.h)

    # Generate the per-target version resource file in two passes:
    # 1. Configure-time generation to replace the values with generator expressions;
    # 2. Generation-time by overwriting the file with the resolved expressions.
    configure_file(bootvid.rc ${CMAKE_CURRENT_BINARY_DIR}/${_target}.rc)
    file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_target}.rc INPUT ${CMAKE_CURRENT_BINARY_DIR}/${_target}.rc) # TARGET ${_target}

    # Create the actual target
    add_library(${_target} MODULE
        ${SOURCE}
        ${CMAKE_CURRENT_BINARY_DIR}/${_target}.rc
        ${CMAKE_CURRENT_BINARY_DIR}/${_target}.def)
    add_pch(${_target} precomp.h "")
    target_compile_definitions(${_target} PRIVATE ${COMPILE_DEFINITIONS} ${_bviddata_DEFINES})

    set_module_type(${_target} kerneldll ENTRYPOINT 0)
    add_importlibs(${_target} ntoskrnl hal)
    add_dependencies(${_target} psdk)

    if(NOT _bviddata_NAME_ON_CD)
        set(_bviddata_NAME_ON_CD "$<TARGET_FILE_NAME:${_target}>")
    endif()
    add_cd_file(TARGET ${_target} DESTINATION reactos/system32 NO_CAB NAME_ON_CD ${_bviddata_NAME_ON_CD} FOR all)
endfunction()

if(ARCH STREQUAL "i386")
## FIXME: Temporary bootvid.dll renames on the CD.
## These should be instead done by the setup program during install.
    if(SARCH STREQUAL "pc98")
        add_bootvid(pc98bvid i386/pc98/pc98bvid.cmake NAME_ON_CD bootvid.dll)
    elseif(SARCH STREQUAL "xbox")
        add_bootvid(xboxbvid i386/xbox/xboxbvid.cmake NAME_ON_CD bootvid.dll)
    else()
        add_bootvid(bootvid i386/pc/vgabvid.cmake)
    endif()
#endif()
#if((ARCH STREQUAL "i386") OR (ARCH STREQUAL "amd64"))
elseif(ARCH STREQUAL "amd64")
    add_bootvid(bootvid i386/pc/vgabvid.cmake)
elseif(ARCH STREQUAL "arm")
    add_bootvid(bootvid arm/armbvid.cmake)
endif()
