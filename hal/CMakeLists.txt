
project(HAL)
if((ARCH STREQUAL "i386") OR (ARCH STREQUAL "amd64"))
    add_subdirectory(halx86)
elseif(ARCH STREQUAL "arm")
    add_subdirectory(halarm)
elseif(ARCH STREQUAL "arm64")

    ## FIXME: Create an import-lib on ARM64 since we don't have a suitable HAL for now.
    spec2def(hal.dll halarm64.spec ADD_IMPORTLIB)

  add_definitions(
    -D_NTHALDLL_
    -D_NTHAL_)

  include_directories(
    include
    ${REACTOS_SOURCE_DIR}/ntoskrnl/include)

    add_library(hal MODULE
        ${CMAKE_CURRENT_BINARY_DIR}/hal_stubs.c
        ${CMAKE_CURRENT_BINARY_DIR}/hal.def)

    target_link_libraries(hal setjmp ${PSEH_LIB})

    add_importlibs(hal ntoskrnl)
    add_dependencies(hal bugcodes xdk psdk asm)
    set_module_type(hal kerneldll ENTRYPOINT HalInitSystem 8)
    add_cd_file(TARGET hal DESTINATION reactos/system32 NO_CAB FOR all)
    if(MSVC)
        target_link_options(hal PRIVATE /ignore:4216 /SECTION:INIT,ERWD)
    endif()

endif()
