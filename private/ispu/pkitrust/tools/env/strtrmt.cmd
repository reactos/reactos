if /i "%1" == "DBG" goto do_dbg
if /i "%1" == "FRE" goto do_fre
goto error_end

:do_fre
D:\nt\idw\remote.exe /s "c:\winnt\system32\cmd.exe /k d:\nt\private\ispu\tools\env\env_ret.cmd %3 %4 %5 %6 %7" %2 /U Administrators /U Remotes /V

goto _end

:do_dbg
D:\nt\idw\remote.exe /s "c:\winnt\system32\cmd.exe /k d:\nt\private\ispu\tools\env\env_dbg.cmd %3 %4 %5 %6 %7" %2 /U Administrators /U Remotes /V

goto _end

:error_end
:_end
