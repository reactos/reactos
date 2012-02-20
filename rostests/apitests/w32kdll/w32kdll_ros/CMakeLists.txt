
spec2def(w32kdll_ros.dll w32kdll_ros.spec)

add_library(w32kdll_ros SHARED
    main.c
    ${CMAKE_CURRENT_BINARY_DIR}/w32kdll_ros.def)

set_entrypoint(w32kdll_ros 0)

target_link_libraries(w32kdll_ros win32ksys)

add_dependencies(w32kdll_ros psdk)
add_importlib_target(w32kdll_ros.spec w32kdll_ros.dll)
