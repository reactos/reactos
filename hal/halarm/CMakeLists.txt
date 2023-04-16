
add_definitions(
    -D_NTHALDLL_
    -D_NTHAL_)

include_directories(
    include
    ${REACTOS_SOURCE_DIR}/ntoskrnl/include)

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
        ${CMAKE_CURRENT_BINARY_DIR}/hal.def
        ${CMAKE_CURRENT_BINARY_DIR}/hal_stubs.c)

    add_importlibs(${_halname} ntoskrnl)
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
include(gic.cmake)

# the ACPI UP GIC hal is default due to existing on qemu (arm32, arm64) with the same GIC revision.
add_hal(hal SOURCES gic/halagic.rc COMPONENTS generic gic) #Acpi, UP #HALAGIC

if(ARCH STREQUAL "arm")
    #ARM32 Specific HALs
    #add_hal(halavic SOURCES vic/halvic.rc COMPONENTS generic vic)
elseif(ARCH STREQUAL "arm64") 
    #ARM64 Specific HALs
endif()