  1 pascal   RegOpenKey(long str ptr) RegOpenKey16
  2 pascal   RegCreateKey(long str ptr) RegCreateKey16
  3 pascal   RegCloseKey(long) RegCloseKey16
  4 pascal   RegDeleteKey(long str) RegDeleteKey16
  5 pascal   RegSetValue(long str long str long) RegSetValue16
  6 pascal   RegQueryValue(long str ptr ptr) RegQueryValue16
  7 pascal   RegEnumKey(long long ptr long) RegEnumKey16
# 8 stub     WEP
  9 pascal -ret16 DragAcceptFiles(word word) DragAcceptFiles16
 11 pascal -ret16 DragQueryFile(word s_word ptr s_word) DragQueryFile16
 12 pascal -ret16 DragFinish(word) DragFinish16
 13 pascal -ret16 DragQueryPoint(word ptr) DragQueryPoint16
 20 pascal -ret16 ShellExecute(word str str str str s_word) ShellExecute16
 21 pascal -ret16 FindExecutable(str str ptr) FindExecutable16
 22 pascal -ret16 ShellAbout(word ptr ptr word) ShellAbout16
 33 pascal -ret16 AboutDlgProc(word word word long) AboutDlgProc16
 34 pascal -ret16 ExtractIcon(word str s_word) ExtractIcon16
 36 pascal -ret16 ExtractAssociatedIcon(word ptr ptr) ExtractAssociatedIcon16
 37 pascal   DoEnvironmentSubst(ptr word) DoEnvironmentSubst16
 38 pascal   FindEnvironmentString(ptr) FindEnvironmentString16
 39 pascal -ret16 InternalExtractIcon(word ptr s_word word) InternalExtractIcon16
 40 pascal -ret16 ExtractIconEx(str word ptr ptr word) ExtractIconEx16
# 98 stub SHL3216_THUNKDATA16
# 99 stub SHL1632_THUNKDATA16

#100   4  0550  HERETHARBETYGARS exported, shared data
#101   8  010e  FINDEXEDLGPROC exported, shared data
101 pascal DllEntryPoint(long word word word long word) SHELL_DllEntryPoint

102 pascal -ret16 RegisterShellHook(word word) RegisterShellHook16
103 pascal   ShellHookProc(word word long) ShellHookProc16

157 stub RESTARTDIALOG
#  166 PICKICONDLG

262 pascal -ret16 DriveType(long) DriveType16

#  263 SH16TO32DRIVEIOCTL
#  264 SH16TO32INT2526
#  300 SHGETFILEINFO
#  400 SHFORMATDRIVE
401 stub SHCHECKDRIVE
#  402 _RUNDLLCHECKDRIVE

#32 WCI
