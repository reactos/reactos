include_directories(../include)

list(APPEND NDIS_DMA_DRV_SOURCE
    ../kmtest_drv/kmtest_standalone.c
    ndis_dma_drv.c)

add_library(ndis_dma_drv MODULE ${NDIS_DMA_DRV_SOURCE})
set_module_type(ndis_dma_drv kernelmodedriver)
target_link_libraries(ndis_dma_drv kmtest_printf ${PSEH_LIB})
add_importlibs(ndis_dma_drv ntoskrnl hal ndis)

# The test relies on the NDIS_MINIPORT_BLOCK layout, which the DDK varies with
# NDIS_WRAPPER, so it is built with the NDIS 5.1 wrapper configuration of
# ndis.sys itself.
target_compile_definitions(ndis_dma_drv PRIVATE
    KMT_STANDALONE_DRIVER
    NDIS_WRAPPER
    NDIS51
    NDIS51_MINIPORT)
#add_pch(ndis_dma_drv ../include/kmt_test.h)
add_rostests_file(TARGET ndis_dma_drv)
