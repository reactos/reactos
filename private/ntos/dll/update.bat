@rem
@rem used to update files used in ole32 nashville build
@rem

@setlocal
@if "%_ntroot%" == "" set _ntroot=\nt
set filelist=ntdllmac.hxx ntprop.cxx ntpropb.cxx propstm.cxx propvar.cxx stgvarb.cxx
@for %%i in (%filelist%) do copy %%i %_ntroot%\private\ole32\stg\props\%%i
@endlocal
