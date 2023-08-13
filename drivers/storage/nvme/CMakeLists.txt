list(APPEND SOURCE
    NvmeStd.c
    NvmeProbe.c
    NvmeIo.c
    NvmeSrb.c
    NvmeModeSense.c
    precomp.h)


add_library(nvme MODULE ${SOURCE} nvme.rc)
add_pch(nvme precomp.h SOURCE)
set_module_type(nvme kernelmodedriver)
add_importlibs(nvme scsiport ntoskrnl hal)
add_cd_file(TARGET nvme DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_driver_inf(nvme nvme.inf)
