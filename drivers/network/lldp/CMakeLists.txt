spec2def(lldp.sys lldp.spec)

include(${REACTOS_SOURCE_DIR}/sdk/cmake/VerosBuildenv.cmake)
set(LLDP_RUST_LIB "${CMAKE_CURRENT_BINARY_DIR}/rust/x86_64-pc-windows-gnu/release/liblldp.a")
set(LLDP_CARGO "${VEROS_CHECKOUT}/tools/rust/cargo/bin/cargo.exe")
set(LLDP_GCC "${VEROS_CHECKOUT}/tools/rosbe/amd64/bin/x86_64-w64-mingw32-gcc.exe")
set(LLDP_GXX "${VEROS_CHECKOUT}/tools/rosbe/amd64/bin/x86_64-w64-mingw32-g++.exe")
set(LLDP_RUST_LINKER "${VEROS_CHECKOUT}/tools/rust/rustup/toolchains/1.99.1-x86_64-pc-windows-gnu/lib/rustlib/x86_64-pc-windows-gnu/bin/self-contained/x86_64-w64-mingw32-gcc.exe")

add_custom_target(lldp_rust
    COMMAND ${CMAKE_COMMAND} -E env
        RUSTUP_HOME=${VEROS_CHECKOUT}/tools/rust/rustup
        CARGO_HOME=${VEROS_CHECKOUT}/tools/rust/cargo
        RUSTUP_TOOLCHAIN=1.99.1-x86_64-pc-windows-gnu
        CC=${LLDP_GCC}
        CXX=${LLDP_GXX}
        CARGO_TARGET_X86_64_PC_WINDOWS_GNU_LINKER=${LLDP_RUST_LINKER}
        RUSTFLAGS=-C\ link-self-contained=yes
        ${LLDP_CARGO} build --release --target x86_64-pc-windows-gnu
            --manifest-path ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml
            --target-dir ${CMAKE_CURRENT_BINARY_DIR}/rust
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml
        ${CMAKE_CURRENT_SOURCE_DIR}/src/lib.rs
    BYPRODUCTS "${LLDP_RUST_LIB}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM)

add_library(lldp MODULE
    lldp.rc
    ${CMAKE_CURRENT_BINARY_DIR}/lldp.def)
target_link_libraries(lldp "${LLDP_RUST_LIB}")
add_dependencies(lldp lldp_rust ndis)
set_module_type(lldp kernelmodedriver)
add_importlibs(lldp ndis ntoskrnl hal)
add_cd_file(TARGET lldp DESTINATION VerOS/system32/drivers FOR all)
