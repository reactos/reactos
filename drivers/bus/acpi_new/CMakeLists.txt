
# Built like the rest of the tree, so shared structures such as ARBITER_INSTANCE match
add_definitions(
    -DNT_TARGET_ID="reactos"
    -DNT_TARGET_NAME="ReactOS")

include_directories(
    include
    reactos
    uacpi-host
    uacpi/include)

list(APPEND UACPI_SOURCE
     uacpi/source/default_handlers.c
     uacpi/source/event.c
     uacpi/source/interpreter.c
     uacpi/source/io.c
     uacpi/source/mutex.c
     uacpi/source/namespace.c
     uacpi/source/notify.c
     uacpi/source/opcodes.c
     uacpi/source/opregion.c
     uacpi/source/osi.c
     uacpi/source/registers.c
     uacpi/source/resources.c
     uacpi/source/shareable.c
     uacpi/source/sleep.c
     uacpi/source/stdlib.c
     uacpi/source/tables.c
     uacpi/source/types.c
     uacpi/source/uacpi.c
     uacpi/source/utilities.c)

add_library(uacpi ${UACPI_SOURCE})
target_compile_definitions(uacpi PUBLIC UACPI_OVERRIDE_LIBC)
add_dependencies(uacpi xdk)

list(APPEND SOURCE
     drivers/applaunch.c
     drivers/button.c
     drivers/devif.c
     drivers/ec.c
     drivers/fan.c
     drivers/processor.c
     drivers/thermal.c
     sys/driver.c
     sys/enum.c
     sys/fdo.c
     sys/filter.c
     sys/glue.c
     sys/guid.c
     sys/hal.c
     sys/interfaces.c
     sys/ioctl.c
     sys/irqarb.c
     sys/irqlib.c
     sys/pdo.c
     sys/power.c
     sys/powerres.c
     sys/reg.c
     sys/resarb.c
     sys/reshub.c
     sys/resource.c
     sys/sleep.c
     uacpi-host/uacpi_host.c
     uacpi-host/uacpi_isr.c)

add_library(acpi MODULE
            ${SOURCE}
            sys/acpi.rc)

set_module_type(acpi kernelmodedriver)
target_link_libraries(acpi uacpi arbiter ntoskrnl_vista wdmguid memcmp ${PSEH_LIB})
add_importlibs(acpi wmilib ntoskrnl hal)
add_cd_file(TARGET acpi DESTINATION reactos/system32/drivers NO_CAB FOR all)

set_target_properties(uacpi acpi PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)

if(MSVC)
    target_compile_options(uacpi PRIVATE /std:c11)
    target_compile_options(acpi PRIVATE /std:c11)

    target_compile_options(uacpi PRIVATE
        /wd4189 /wd4244 /wd4267 /wd4310 /wd4324 /wd4389 /wd4701 /wd4703)
endif()
