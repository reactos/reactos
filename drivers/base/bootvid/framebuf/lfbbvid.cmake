
set(MODULE_HEADER ${CMAKE_CURRENT_LIST_DIR}/framebuf.h) # For precomp.h
list(APPEND SOURCE
    ${MODULE_HEADER}
    ${CMAKE_CURRENT_LIST_DIR}/bootvid.c)

set(REACTOS_STR_FILE_DESCRIPTION "Generic Linear FrameBuffer Boot Driver")
