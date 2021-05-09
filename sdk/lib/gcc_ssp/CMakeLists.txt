
add_library(gcc_ssp_nt OBJECT gcc_ssp.c)

add_library(gcc_ssp_msvcrt OBJECT gcc_ssp.c)
target_compile_definitions(gcc_ssp_msvcrt PRIVATE _GCC_SSP_MSVCRT_)

add_library(gcc_ssp_win32k OBJECT gcc_ssp.c)
target_compile_definitions(gcc_ssp_win32k PRIVATE _GCC_SSP_WIN32K_)

add_library(gcc_ssp_videoprt OBJECT gcc_ssp.c)
target_compile_definitions(gcc_ssp_videoprt PRIVATE _GCC_SSP_VIDEOPRT_)

add_library(gcc_ssp_scsiport OBJECT gcc_ssp.c)
target_compile_definitions(gcc_ssp_scsiport PRIVATE _GCC_SSP_SCSIPORT_)
