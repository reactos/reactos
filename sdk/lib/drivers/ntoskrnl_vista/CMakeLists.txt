add_definitions(-DUNICODE -D_UNICODE -D__NTOSKRNL__ -D_NTOSKRNL_ -D_NTSYSTEM_)

list(APPEND SOURCE
    etw.c
    fsrtl.c
    io.c
    po.c
    ke.c
    ${REACTOS_SOURCE_DIR}/sdk/lib/rtl/utf8.c)

add_library(ntoskrnl_vista ${SOURCE})
target_link_libraries(ntoskrnl_vista PRIVATE pseh) # rtl_vista)
add_dependencies(ntoskrnl_vista bugcodes xdk)
set_ntddi_defines(ntoskrnl_vista vista)

target_compile_definitions(ntoskrnl_vista PUBLIC NTKRNLVISTA)
