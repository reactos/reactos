
add_library(tapiui SHARED tapiui.rc)
set_entrypoint(tapiui 0)
add_dependencies(tapiui psdk buildno_header)

add_cab_target(tapiui 1)
