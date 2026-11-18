spec2def(rtl8139.sys rtl8139.spec)

include(${REACTOS_SOURCE_DIR}/sdk/cmake/VerosBuildenv.cmake)
set(RTL_RUST_LIB "${CMAKE_CURRENT_BINARY_DIR}/rust/x86_64-pc-windows-gnu/release/librtl8139.a")
set(RTL_CARGO "${VEROS_CHECKOUT}/tools/rust/cargo/bin/cargo.exe")
set(RTL_GCC "${VEROS_CHECKOUT}/tools/rosbe/amd64/bin/x86_64-w64-mingw32-gcc.exe")
set(RTL_GXX "${VEROS_CHECKOUT}/tools/rosbe/amd64/bin/x86_64-w64-mingw32-g++.exe")
set(RTL_RUST_LINKER "${VEROS_CHECKOUT}/tools/rust/rustup/toolchains/1.99.1-x86_64-pc-windows-gnu/lib/rustlib/x86_64-pc-windows-gnu/bin/self-contained/x86_64-w64-mingw32-gcc.exe")

add_custom_target(rtl8139_rust
    COMMAND ${CMAKE_COMMAND} -E env
        RUSTUP_HOME=${VEROS_CHECKOUT}/tools/rust/rustup
        CARGO_HOME=${VEROS_CHECKOUT}/tools/rust/cargo
        RUSTUP_TOOLCHAIN=1.99.1-x86_64-pc-windows-gnu
        CC=${RTL_GCC}
        CXX=${RTL_GXX}
        CARGO_TARGET_X86_64_PC_WINDOWS_GNU_LINKER=${RTL_RUST_LINKER}
        RUSTFLAGS=-C\ link-self-contained=yes
        ${RTL_CARGO} build --release --target x86_64-pc-windows-gnu
            --manifest-path ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml
            --target-dir ${CMAKE_CURRENT_BINARY_DIR}/rust
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml
        ${CMAKE_CURRENT_SOURCE_DIR}/src/lib.rs
    BYPRODUCTS "${RTL_RUST_LIB}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM)

add_library(rtl8139 MODULE
    rtl8139.rc
    ${CMAKE_CURRENT_BINARY_DIR}/rtl8139.def)
target_link_libraries(rtl8139 "${RTL_RUST_LIB}")
add_dependencies(rtl8139 rtl8139_rust ndis)
set_module_type(rtl8139 kernelmodedriver)
add_importlibs(rtl8139 ndis ntoskrnl hal)
add_cd_file(TARGET rtl8139 DESTINATION VerOS/system32/drivers FOR all)
