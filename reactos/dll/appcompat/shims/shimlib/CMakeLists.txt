
list(APPEND SOURCE
    shimdbgsupp.c
    shimlib.c
    shimlib.h
    # These .inl functions are included so they show up in vs
    setup_shim.inl
    implement_shim.inl)

add_library(shimlib ${SOURCE})
add_dependencies(shimlib xdk)
