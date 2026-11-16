
## Temp HACK: Must match VER_PRODUCT(MAJOR|MINOR)VERSION used by ntoskrnl,
## until they match consistently with NTDDI_VERSION and DLL_EXPORT_VERSION.
set(BOOTVID_EXPORTS_VERSION 0x502)
add_definitions(-DBOOTVID_EXPORTS_VERSION=${BOOTVID_EXPORTS_VERSION})

function(add_bootvid _target _configfile)
    cmake_parse_arguments(_bviddata "" "NAME_ON_CD" "SOURCES" ${ARGN})

    # Handle the spec file for the dll name
    spec2def(${_target}.dll bootvid.spec VERSION ${BOOTVID_EXPORTS_VERSION} ADD_IMPORTLIB)

    # Set default values for the version resource file
    set(REACTOS_STR_INTERNAL_NAME "$<TARGET_NAME:${_target}>")
    set(REACTOS_STR_ORIGINAL_FILENAME "$<TARGET_FILE_NAME:${_target}>")

    # Include the per-target definitions:
    # NO_VIDEO_GRAPHICS, COMPILE_DEFINITIONS, MODULE_HEADER, and SOURCE
    include(${_configfile})
    list(APPEND SOURCE ${_bviddata_SOURCES})

    # Manage the module-specific header for precomp.h
    if(MODULE_HEADER)
        #get_filename_component(MODULE_HEADER "${MODULE_HEADER}" ABSOLUTE)
        file(RELATIVE_PATH MODULE_HEADER "${CMAKE_CURRENT_SOURCE_DIR}" "${MODULE_HEADER}")
        #string(REPLACE "\\" "/" MODULE_HEADER "${MODULE_HEADER}")
        file(TO_CMAKE_PATH "${MODULE_HEADER}" MODULE_HEADER)
        list(APPEND COMPILE_DEFINITIONS "MODULE_HEADER=\"${MODULE_HEADER}\"")
    endif()

    # Add the common sources, with or without graphics support
    add_asm_files(SOURCE vidinit.S)
    list(APPEND SOURCE vidinit.c console.c precomp.h)
    if(NO_VIDEO_GRAPHICS)
        list(APPEND COMPILE_DEFINITIONS NO_VIDEO_GRAPHICS)
    else()
        list(APPEND SOURCE common.c fontdata.c)
    endif()

    # Generate the per-target version resource file in two passes:
    # 1. Configure-time generation to replace the values with generator expressions;
    # 2. Generation-time by overwriting the file with the resolved expressions.
    configure_file(bootvid.rc ${CMAKE_CURRENT_BINARY_DIR}/${_target}.rc)
    file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_target}.rc
                   INPUT ${CMAKE_CURRENT_BINARY_DIR}/${_target}.rc) # TARGET ${_target}

    # Create the actual target
    add_library(${_target} MODULE
        ${SOURCE}
        ${CMAKE_CURRENT_BINARY_DIR}/${_target}.rc
        ${CMAKE_CURRENT_BINARY_DIR}/${_target}.def)
    add_pch(${_target} precomp.h "vidinit.c")
    target_compile_definitions(${_target} PRIVATE ${COMPILE_DEFINITIONS})

    set_module_type(${_target} kerneldll ENTRYPOINT 0)
    add_importlibs(${_target} ntoskrnl hal)
    add_dependencies(${_target} psdk)

    # Reduce the required subsystem to Windows 2000 (NT 5) for i386 builds only.
    if(ARCH STREQUAL "i386")
        if(MSVC)
            # NOTE: We cannot use the following command:
            #   target_link_options(${_target} PRIVATE "/SUBSYSTEM:NATIVE,5.00")
            # because it would act at the level of the LINK.EXE linker flags,
            # which only accepts a subsystem version >= 5.10 (Windows XP+) on
            # latest MSVC versions.
            # So to work around this problem, we use a post-build command by
            # employing EDITBIN.EXE that does not check the subsystem version.
            #
            add_custom_command(
                TARGET ${_target} POST_BUILD
                COMMAND editbin.exe /NOLOGO /SUBSYSTEM:NATIVE,5.00 $<TARGET_FILE:${_target}> > NUL
                VERBATIM)
        else()
            # The binutils linker does not set a lower limit on the subsystem.
            # Otherwise we would use: objcopy --subsystem native:5.00 $<TARGET_FILE:${_target}>
            target_link_options(${_target} PRIVATE "-Wl,--subsystem,native:5.00")
        endif()
    endif()

    if(NOT _bviddata_NAME_ON_CD)
        set(_bviddata_NAME_ON_CD "$<TARGET_FILE_NAME:${_target}>")
    endif()
    add_cd_file(TARGET ${_target} DESTINATION reactos/system32 NO_CAB NAME_ON_CD ${_bviddata_NAME_ON_CD} FOR all)
endfunction()

if(ARCH STREQUAL "i386")
## FIXME: Temporary bootvid.dll renames on the CD.
## These should be done instead by the setup program during install.
    if(SARCH STREQUAL "pc98")
        set(PC98BVID_NAME_ON_CD bootvid.dll)
        set(VGABVID_NAME_ON_CD vgabvid.dll)
    elseif(SARCH STREQUAL "xbox")
        set(XBOXBVID_NAME_ON_CD bootvid.dll)
        set(VGABVID_NAME_ON_CD vgabvid.dll)
    endif()
    add_bootvid(pc98bvid i386/pc98/pc98bvid.cmake NAME_ON_CD ${PC98BVID_NAME_ON_CD})
    add_bootvid(xboxbvid i386/xbox/xboxbvid.cmake NAME_ON_CD ${XBOXBVID_NAME_ON_CD})
endif()
if((ARCH STREQUAL "i386") OR (ARCH STREQUAL "amd64"))
    add_bootvid(bootvid i386/pc/vgabvid.cmake NAME_ON_CD ${VGABVID_NAME_ON_CD})
elseif(ARCH STREQUAL "arm")
    add_bootvid(bootvid arm/armbvid.cmake)
endif()
add_bootvid(lfbbvid framebuf/lfbbvid.cmake)
