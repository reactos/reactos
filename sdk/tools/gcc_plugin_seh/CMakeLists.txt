
add_host_module(gcc_plugin_seh main.cpp)
target_include_directories(gcc_plugin_seh PRIVATE ${GCC_PLUGIN_DIR}/include)
set_target_properties(gcc_plugin_seh PROPERTIES POSITION_INDEPENDENT_CODE ON)

if (CMAKE_HOST_SYSTEM MATCHES "Windows")
    # On windows, GCC plugins need to be linked to the main executable
    # This means that there must be a plugin for both GCC & G++
    target_link_libraries(gcc_plugin_seh PRIVATE ${GCC_PLUGIN_DIR}/cc1.exe.a)
    add_host_module(g++_plugin_seh $<TARGET_OBJECTS:gcc_plugin_seh>)
    target_link_libraries(g++_plugin_seh PRIVATE ${GCC_PLUGIN_DIR}/cc1plus.exe.a)
endif()
