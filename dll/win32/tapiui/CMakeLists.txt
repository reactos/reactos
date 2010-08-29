
set(CMAKE_RC_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

add_library(tapiui SHARED tapiui.rc)

set_target_properties(tapiui PROPERTIES LINK_FLAGS "-Wl,-entry,0")

add_dependencies(tapiui psdk)
