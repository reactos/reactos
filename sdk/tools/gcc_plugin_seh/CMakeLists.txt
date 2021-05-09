
add_host_module(gcc_plugin_seh main.cpp)
target_include_directories(gcc_plugin_seh PRIVATE ${GCC_PLUGIN_DIR}/include)
set_target_properties(gcc_plugin_seh PROPERTIES POSITION_INDEPENDENT_CODE ON)
