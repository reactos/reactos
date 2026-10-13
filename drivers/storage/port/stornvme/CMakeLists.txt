file(GLOB STORNVME_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

nt_add_driver(stornvme ${STORNVME_SOURCES} stornvme.rc)

target_link_libraries(stornvme PRIVATE mdr_shim WDK::storport)
