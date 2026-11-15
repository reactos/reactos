spec2def(ndis.sys ndis.spec ADD_IMPORTLIB)

get_filename_component(VEROS_CHECKOUT "${REACTOS_SOURCE_DIR}" DIRECTORY)
set(NDIS_RUST_LIB "${CMAKE_CURRENT_BINARY_DIR}/rust/x86_64-pc-windows-gnu/release/libndis.a")
set(NDIS_CARGO "${VEROS_CHECKOUT}/tools/rust/cargo/bin/cargo.exe")
set(NDIS_GCC "${VEROS_CHECKOUT}/tools/rosbe/amd64/bin/x86_64-w64-mingw32-gcc.exe")
set(NDIS_GXX "${VEROS_CHECKOUT}/tools/rosbe/amd64/bin/x86_64-w64-mingw32-g++.exe")
set(NDIS_RUST_LINKER "${VEROS_CHECKOUT}/tools/rust/rustup/toolchains/1.99.1-x86_64-pc-windows-gnu/lib/rustlib/x86_64-pc-windows-gnu/bin/self-contained/x86_64-w64-mingw32-gcc.exe")

add_custom_target(ndis_rust
    COMMAND ${CMAKE_COMMAND} -E env
        RUSTUP_HOME=${VEROS_CHECKOUT}/tools/rust/rustup
        CARGO_HOME=${VEROS_CHECKOUT}/tools/rust/cargo
        RUSTUP_TOOLCHAIN=1.99.1-x86_64-pc-windows-gnu
        CC=${NDIS_GCC}
        CXX=${NDIS_GXX}
        CARGO_TARGET_X86_64_PC_WINDOWS_GNU_LINKER=${NDIS_RUST_LINKER}
        RUSTFLAGS=-C\ link-self-contained=yes
        ${NDIS_CARGO} build --release --target x86_64-pc-windows-gnu
            --manifest-path ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml
            --target-dir ${CMAKE_CURRENT_BINARY_DIR}/rust
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml
        ${CMAKE_CURRENT_SOURCE_DIR}/src/lib.rs
        ${CMAKE_CURRENT_SOURCE_DIR}/src/pool.rs
        ${CMAKE_CURRENT_SOURCE_DIR}/src/register.rs
        ${CMAKE_CURRENT_SOURCE_DIR}/src/filter.rs
        ${CMAKE_CURRENT_SOURCE_DIR}/src/protocol.rs
    BYPRODUCTS "${NDIS_RUST_LIB}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM)

add_library(ndis MODULE
    ndis.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ndis.def)
target_link_libraries(ndis "${NDIS_RUST_LIB}")
add_dependencies(ndis ndis_rust)
set_module_type(ndis kernelmodedriver)
add_importlibs(ndis ntoskrnl hal)
add_cd_file(TARGET ndis DESTINATION VerOS/system32/drivers FOR all)
