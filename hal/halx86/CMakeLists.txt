
add_definitions(
    -D_NTHALDLL_
    -D_NTHAL_)

include_directories(
    include
    ${REACTOS_SOURCE_DIR}/ntoskrnl/include
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/libs/fast486)

function(add_hal _halname)
    cmake_parse_arguments(_haldata "" "" "SOURCES;COMPONENTS" ${ARGN})

    # Handle the spec file for the dll name
    spec2def(${_halname}.dll ../hal.spec ADD_IMPORTLIB)

    # Create the actual target
    foreach(_component ${_haldata_COMPONENTS})
        list(APPEND _haldata_SOURCES "$<TARGET_OBJECTS:lib_hal_${_component}>")
    endforeach()
    add_library(${_halname} MODULE
        ${_haldata_SOURCES}
        ${CMAKE_CURRENT_BINARY_DIR}/hal.def)

    if(${_halname} STREQUAL "hal")
        target_link_libraries(${_halname} libcntpr arbiter)
    else()
        target_link_libraries(${_halname} libcntpr)
    endif()

    add_importlibs(${_halname} ntoskrnl)
    #add_pch(${_halname} include/hal.h)
    add_dependencies(${_halname} psdk asm)
    set_module_type(${_halname} kerneldll ENTRYPOINT HalInitSystem 8)
    add_cd_file(TARGET ${_halname} DESTINATION reactos/system32 NO_CAB FOR all)
    if(MSVC)
        target_link_options(${_halname} PRIVATE /ignore:4216 /SECTION:INIT,ERWD)
    else()
        target_link_libraries(${_halname} -lgcc)
    endif()
endfunction()

# The components
include(generic.cmake)
include(acpi.cmake)
include(apic.cmake)
include(up.cmake)
include(smp.cmake)

if(ARCH STREQUAL "i386")
    include(pcidata.cmake)
    include(legacy.cmake)
    include(pic.cmake)
    include(xbox.cmake)
    include(pc98.cmake)
    add_subdirectory(minihal)

    remove_definitions(-DSARCH_XBOX)
    remove_definitions(-DSARCH_PC98)

    # hal
    add_hal(hal SOURCES pic/halpic.rc COMPONENTS generic legacy up pic)
    add_hal(halacpi SOURCES acpi/halacpi.rc COMPONENTS generic acpi up pic)
    add_hal(halaacpi SOURCES apic/halaacpi.rc COMPONENTS generic acpi up apic)
    add_hal(halapic SOURCES apic/halapic.rc COMPONENTS generic legacy up apic)
    add_hal(halxbox SOURCES xbox/halxbox.rc COMPONENTS xbox up)
    add_hal(halpc98 SOURCES pc98/halpc98.rc COMPONENTS pc98 up)

    #add_hal(halmacpi SOURCES acpi/halacpi.rc COMPONENTS generic acpi smp pic)

elseif(ARCH STREQUAL "amd64")

    list(APPEND HAL_SOURCE
        amd64/x86bios.c)

    add_hal(hal SOURCES ${HAL_SOURCE} COMPONENTS generic acpi up apic)
    target_link_libraries(hal fast486)

    add_hal(halmp SOURCES ${HAL_SOURCE} COMPONENTS generic acpi smp apic)
    target_link_libraries(halmp fast486)

endif()
