
list(APPEND SOURCE
    main.c
    txt.c
    nls.c)

add_host_tool(txt2nls ${SOURCE})
