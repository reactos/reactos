
list(APPEND SOURCE
    lang.c
    mcl.c
    utils.c
    wmc.c
    write.c
    mcy.tab.c)

add_executable(wmc ${SOURCE})

target_link_libraries(wmc unicode)
