
set_cpp()

spec2def(ksproxy.ax ksproxy.spec)

add_library(ksproxy SHARED
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
    ksproxy.rc
    mediasample.cpp
    node.cpp
    output_pin.cpp
    proxy.cpp
    qualityforward.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/ksproxy.def)

set_module_type(ksproxy win32dll ENTRYPOINT 0)
set_target_properties(ksproxy PROPERTIES SUFFIX ".ax")

if(MSVC)
    add_target_compile_flags(ksproxy "/GR-")
else()
    add_target_compile_flags(ksproxy "-fno-exceptions -fno-rtti")
endif()

target_link_libraries(ksproxy strmiids)

add_importlibs(ksproxy
    advapi32
    ole32
    setupapi
    ksuser
    msvcrt
    kernel32
    ntdll)

add_cd_file(TARGET ksproxy DESTINATION reactos/system32 FOR all)
