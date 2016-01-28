
set_cpp(WITH_STL)
spec2def(ksproxy.ax ksproxy.spec)

list(APPEND SOURCE
    allocator.cpp
    basicaudio.cpp
    classfactory.cpp
    clockforward.cpp
    cvpconfig.cpp
    cvpvbiconfig.cpp
    datatype.cpp
    enum_mediatypes.cpp
    enumpins.cpp
    input_pin.cpp
    interface.cpp
    ksproxy.cpp
    mediasample.cpp
    node.cpp
    output_pin.cpp
    proxy.cpp
    qualityforward.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/ksproxy.def)

add_library(ksproxy SHARED ${SOURCE} ksproxy.rc)
set_module_type(ksproxy win32dll)
set_target_properties(ksproxy PROPERTIES SUFFIX ".ax")
target_link_libraries(ksproxy strmiids)
add_importlibs(ksproxy advapi32 ole32 setupapi ksuser msvcrt kernel32 ntdll)
add_dependencies(ksproxy dxsdk)
add_cd_file(TARGET ksproxy DESTINATION reactos/system32 FOR all)
