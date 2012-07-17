
add_definitions(
    -D_NTHALDLL_
    -D_NTHAL_)

include_directories(
    include
    ${REACTOS_SOURCE_DIR}/ntoskrnl/include)

list(APPEND HAL_GENERIC_SOURCE
    generic/beep.c
    generic/cmos.c
    generic/display.c
    generic/dma.c
    generic/drive.c
    generic/halinit.c
    generic/memory.c
    generic/misc.c
    generic/reboot.c
    generic/sysinfo.c
    generic/usage.c)

if(ARCH STREQUAL i386)
    list(APPEND HAL_GENERIC_SOURCE
        generic/bios.c
        generic/portio.c)
endif()

if(CMAKE_VERSION VERSION_GREATER 2.8.7)
    add_library(lib_hal_generic OBJECT ${HAL_GENERIC_SOURCE})
else()
    add_library(lib_hal_generic ${HAL_GENERIC_SOURCE})
    add_dependencies(lib_hal_generic asm)
endif()

add_pch(lib_hal_generic include/hal.h)

list(APPEND HAL_LEGACY_SOURCE
    legacy/bus/bushndlr.c
    legacy/bus/cmosbus.c
    legacy/bus/isabus.c
    legacy/bus/pcibus.c
    legacy/bus/pcidata.c
    legacy/bus/sysbus.c
    legacy/bussupp.c
    legacy/halpnpdd.c
    legacy/halpcat.c)

if(CMAKE_VERSION VERSION_GREATER 2.8.7)
    add_library(lib_hal_legacy OBJECT ${HAL_LEGACY_SOURCE})
else()
    add_library(lib_hal_legacy ${HAL_LEGACY_SOURCE})
endif()

#add_pch(lib_hal_legacy include/hal.h)

if(NOT CMAKE_VERSION VERSION_GREATER 2.8.7)
    target_link_libraries(lib_hal_legacy lib_hal_generic)
endif()

list(APPEND HAL_ACPI_SOURCE
    acpi/halacpi.c
    acpi/halpnpdd.c
    acpi/busemul.c
    legacy/bus/pcibus.c)

if(CMAKE_VERSION VERSION_GREATER 2.8.7)
    add_library(lib_hal_acpi OBJECT ${HAL_ACPI_SOURCE})
else()
    add_library(lib_hal_acpi ${HAL_ACPI_SOURCE})
endif()
#add_pch(lib_hal_acpi include/hal.h)

if(NOT CMAKE_VERSION VERSION_GREATER 2.8.7)
    target_link_libraries(lib_hal_acpi lib_hal_generic)
endif()

list(APPEND HAL_UP_SOURCE
    generic/spinlock.c
    up/processor.c)

if(CMAKE_VERSION VERSION_GREATER 2.8.7)
    add_library(lib_hal_up OBJECT ${HAL_UP_SOURCE})
endif()

list(APPEND HAL_PIC_SOURCE
    generic/profil.c
    generic/timer.c
    generic/systimer.S
    generic/trap.S
    up/halinit_up.c
    up/pic.c)

if(CMAKE_VERSION VERSION_GREATER 2.8.7)
    add_library(lib_hal_pic OBJECT ${HAL_PIC_SOURCE})
endif()

list(APPEND HAL_APIC_SOURCE
    apic/apic.c
    apic/apictimer.c
    apic/apictrap.S
    apic/halinit_apic.c
    apic/rtctimer.c
    apic/tsc.c
    apic/tsccal.S)

if(ARCH STREQUAL i386)
    list(APPEND MINI_HAL_SOURCE
        generic/portio.c
        generic/systimer.S
        legacy/bus/bushndlr.c
        legacy/bus/cmosbus.c
        legacy/bus/isabus.c
        legacy/bus/pcibus.c
        legacy/bus/sysbus.c
        legacy/bussupp.c
        generic/beep.c
        generic/bios.c
        generic/cmos.c
        generic/dma.c
        generic/display.c
        generic/drive.c
        generic/misc.c
        generic/profil.c
        generic/reboot.c
        generic/spinlock.c
        generic/sysinfo.c
        generic/timer.c
        generic/usage.c
        up/halinit_mini.c
        up/pic.c
        up/processor.c)

    spec2def(hal.dll ../hal.spec ADD_IMPORTLIB)

    # hal
    if(CMAKE_VERSION VERSION_GREATER 2.8.7)
        add_library(hal SHARED
            $<TARGET_OBJECTS:lib_hal_generic>
            $<TARGET_OBJECTS:lib_hal_legacy>
            $<TARGET_OBJECTS:lib_hal_up>
            $<TARGET_OBJECTS:lib_hal_pic>
            generic/v86.S
            up/halup.rc
            ${CMAKE_CURRENT_BINARY_DIR}/hal.def)
    else()
        add_library(hal SHARED
            ${HAL_UP_SOURCE}
            ${HAL_PIC_SOURCE}
            generic/v86.S
            up/halup.rc
            ${CMAKE_CURRENT_BINARY_DIR}/hal.def)
    endif()

    if(NOT CMAKE_VERSION VERSION_GREATER 2.8.7)
        target_link_libraries(hal lib_hal_generic lib_hal_legacy)
    endif()
    target_link_libraries(hal libcntpr)
    add_importlibs(hal ntoskrnl)
    #add_pch(hal include/hal.h)
    add_dependencies(hal psdk bugcodes)
    set_entrypoint(hal HalInitSystem 8)
    set_subsystem(hal native)
    set_image_base(hal 0x00010000)
    add_cd_file(TARGET hal DESTINATION reactos/system32 NO_CAB FOR all)
    if(MSVC)
        add_target_link_flags(hal "/ignore:4216 /ignore:4078")
    endif()

    # hal acpi
    spec2def(halacpi.dll ../hal.spec)
    if(CMAKE_VERSION VERSION_GREATER 2.8.7)
        add_library(halacpi SHARED
            $<TARGET_OBJECTS:lib_hal_generic>
            $<TARGET_OBJECTS:lib_hal_acpi>
            $<TARGET_OBJECTS:lib_hal_up>
            $<TARGET_OBJECTS:lib_hal_pic>
            generic/v86.S
            acpi/halacpi.rc
            ${CMAKE_CURRENT_BINARY_DIR}/halacpi.def)
    else()
        add_library(halacpi SHARED
            ${HAL_UP_SOURCE}
            ${HAL_PIC_SOURCE}
            generic/v86.S
            acpi/halacpi.rc
            ${CMAKE_CURRENT_BINARY_DIR}/halacpi.def)
    endif()

    if(NOT CMAKE_VERSION VERSION_GREATER 2.8.7)
        target_link_libraries(halacpi lib_hal_generic lib_hal_acpi)
    endif()
    target_link_libraries(halacpi libcntpr)
    add_importlibs(halacpi ntoskrnl)
    add_dependencies(halacpi psdk bugcodes)
    set_entrypoint(halacpi HalInitSystem@8)
    set_subsystem(halacpi native)
    set_image_base(halacpi 0x00010000)
    add_target_compile_definitions(halacpi CONFIG_ACPI)
    add_cd_file(TARGET halacpi DESTINATION reactos/system32 NO_CAB FOR all)
    if(MSVC)
        add_target_link_flags(halacpi "/ignore:4216 /ignore:4078")
    endif()

    # mini_hal
    add_library(mini_hal ${MINI_HAL_SOURCE})
    add_target_compile_definitions(mini_hal _BLDR_ _MINIHAL_)
    add_dependencies(mini_hal psdk bugcodes asm)

elseif(ARCH STREQUAL amd64)

    spec2def(hal.dll ../hal.spec ADD_IMPORTLIB)

    list(APPEND HAL_SOURCE
        ${HAL_GENERIC_SOURCE}
        ${HAL_ACPI_SOURCE}
        ${HAL_APIC_SOURCE}
        generic/spinlock.c
        amd64/x86bios.c
        amd64/halinit.c
        amd64/processor.c
        ${CMAKE_CURRENT_BINARY_DIR}/hal.def)

    # hal
    add_library(hal SHARED ${HAL_SOURCE})
    add_pch(hal include/hal.h)
    add_dependencies(hal psdk bugcodes)

    set_entrypoint(hal HalInitSystem)
    set_subsystem(hal native)
    set_image_base(hal 0x00010000)
    add_importlibs(hal ntoskrnl)
    target_link_libraries(hal libcntpr)
    if(MSVC)
        add_target_link_flags(hal "/ignore:4216")
    endif()

    add_cd_file(TARGET hal DESTINATION reactos/system32 NO_CAB FOR all)

endif()


