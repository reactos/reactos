1    stub     WEP
2    pascal -ret16 IpOpen(str ptr) IpOpen16
3    stub     IpOpenAppend #(str word)
4    pascal -ret16 IpClose(word) IpClose16
5    stub     IpGetLongField #(word ptr word ptr)
6    stub     IpGetStringField #(word ptr word ptr word ptr)
7    stub     IpFindFirstLine #(word str str ptr)
8    stub     IpGetLineCount #(word str ptr)
9    stub     IpGetFieldCount #(word ptr ptr)
10   stub     IpGetIntField #(word ptr word ptr)
11   stub     IpFindNextLine #(word ptr)
12   stub     IpGetFileName #(word ptr word)
13   pascal -ret16 VcpQueueCopy(str str str str word word ptr word long) VcpQueueCopy16
14   stub     NOAUTORUNWNDPROC
15   stub     __DEBUGMSG
16   stub     __ASSERTMSG
17   pascal -ret16 VcpQueueDelete(str str word long) VcpQueueDelete16
18   stub     TpOpenFile #(str ptr word)
19   stub     TpCloseFile #(word)
20   stub     TpOpenSection #(word ptr str word)
21   stub     TpCloseSection #(word)
22   stub     TpCommitSection #(word word str word)
23   stub     TpGetLine #(word str str word word ptr)
24   stub     TpGetNextLine #(word str str ptr)
25   stub     TpInsertLine #(word str str word word word)
26   stub     TpReplaceLine #(word str str word word word)
27   stub     TpDeleteLine #(word word word word)
28   stub     TpMoveLine #(word word word word word word)
29   stub     TpGetLineContents #(word ptr word ptr word word word)
30   stub     TpGetPrivateProfileString #(str str str ptr word str)
31   stub     TpWritePrivateProfileString #(str str str str)
32   stub     TpGetProfileString #(str str str ptr word)
33   pascal -ret16 CtlSetLdd(ptr) CtlSetLdd16
34   pascal -ret16 CtlGetLdd(ptr) CtlGetLdd16
35   pascal -ret16 CtlFindLdd(ptr) CtlFindLdd16
36   pascal -ret16 CtlAddLdd(ptr) CtlAddLdd16
37   pascal -ret16 CtlDelLdd(word) CtlDelLdd16
38   pascal -ret16 CtlGetLddPath(word ptr) CtlGetLddPath16
39   stub     SURegCloseKey #(word)
40   stub     SURegCreateKey #(word)
41   stub     SURegDeleteKey #(word str)
42   stub     SURegDeleteValue #(word str)
43   stub     SURegEnumKey #(word long ptr long)
44   stub     SURegEnumValue #(word long str ptr ptr ptr ptr ptr)
45   stub     SURegFlush #()
46   stub     SURegInit #()
47   pascal   SURegOpenKey(word str ptr) SURegOpenKey
48   stub     SURegQueryValue
49   stub     SURegQueryValue16 #(word str ptr ptr)
50   pascal   SURegQueryValueEx(long str ptr ptr ptr ptr) SURegQueryValueEx
51   stub     SURegSetValue
52   stub     SURegSetValue16 #(word str long ptr long)
53   stub     SURegSetValueEx #(word str long long ptr long)
54   stub     SURegSaveKey #(word str ptr)
55   stub     SURegLoadKey #(word str str)
56   stub     SURegUnLoadKey #(word str)
60   stub     DiskInfoFromLdid #(word ptr)
61   pascal   suErrorToIds(word word) suErrorToIds16
62   pascal -ret16 TPWriteProfileString(str str str) TPWriteProfileString16
63   stub     SURPLSETUP
# does SUSTORELDIDPATH set the path of an LDID in the registry ?
64   stub     SUSTORELDIDPATH
65   stub     WILDCARDSTRCMPI
101  pascal -ret16 GenInstall(word str word) GenInstall16
102  stub     GenWinInitRename #(str str word)
103  pascal   GenFormStrWithoutPlaceHolders(str str word) GenFormStrWithoutPlaceHolders16
104  stub     SETUPX
105  stub     CfgSetupMerge #(word)
106  stub     INITDEPENDANTLDIDS
107  stub     CFGOBJFINDKEYCMD
108  stub     GenSURegSetValueEx
109  stub     GENINSTALLWITHQUEUE
110  stub     GenInstallEx #(word str word word ptr long)
111  stub     GenCopyLogConfig2Reg #(word word str)
112  stub     SUGetSetSetupFlags #(ptr word)
114  stub     CFGPARSELINE # returns array
115  stub     CFGSETAUTOPROCESS
116  stub     CFGOBJTOSTR
117  stub     CFGLNTOOBJ
118  stub     MATCHCMDEXT
119  stub     IpFindNextMatchLine #(word str ptr)
120  stub     P_SETDEFAULTOPTION
121  stub     CFGCLEANBOOT
122  stub     CFGMATCHCMDEXT
123  stub     CFGWASFILEUPDATED
124  stub     AUTOMATCHCMDEXT
125  stub     P_VALIDATEOC
126  stub     GENMAPROOTREGSTR2KEY
127  stub     P_CDROMOC
128  stub     P_MEDIAOC
129  stub     CFGCLEAN1STBOOT
130  stub     suFormatMessage
131  stub     suvFormatMessage #(word str str word ptr)
132  stub     suFormatMessageBox
#133  stub     suHelp # W98SE conflict !!
135  stub     suHelp #(word word)
#135  stub     P_WEBTVOC # W98SE conflict !!
136  stub     P_WBEMOC
137  stub     P_THEMESOC
138  stub     P_IMAGINGOC
139  stub     P_SCHEMESOC
140  stub     suVerConflict #(word ptr word ptr)
141  stub     suVerConflictInit #(word)
142  stub     suVerConflictTerm #(ptr)
# Emergency Boot Disk
143  stub     suCreateEBD #(word ptr long)
144  stub     suCopyToEBD
145  stub     sxIsMSDOS7Running #()
150  stub     DS_INIT
151  stub     DS_DESTROY
152  stub     DS_SSYNCDRIVES
153  stub     DS_GETDRIVEDATA
154  stub     DS_ADDSECTION
155  stub     DS_ENABLESECTION
156  stub     DS_DISABLESECTION
157  stub     DS_SETSWAPSIZE
158  stub     DS_SETREQUIREDPAD
159  stub     DS_SETAVAILABLEPAD
160  stub     SXUPDATEDS
170  stub     SUSETMEM
171  stub     WriteDMFBootData #(word ptr word)
200  pascal   VcpOpen(segptr ptr) VcpOpen16
201  pascal   VcpClose(word str) VcpClose16
202  pascal -ret16 vcpDefCallbackProc(ptr word word long long) vcpDefCallbackProc16
203  stub     vcpEnumFiles #(ptr long)
204  pascal -ret16 VcpQueueRename(str str str str word word long) VcpQueueRename16
205  pascal -ret16 vsmGetStringName(word ptr word) vsmGetStringName16
206  pascal -ret16 vsmStringDelete(word) vsmStringDelete16
207  pascal -ret16 vsmStringAdd(str) vsmStringAdd16
208  pascal   vsmGetStringRawName(word) vsmGetStringRawName16
209  stub     IpSaveRestorePosition #(word word)
210  pascal -ret16 IpGetProfileString(word str str ptr word) IpGetProfileString16
211  stub     IpOpenEx #(str ptr word)
212  stub     IpOpenAppendEx #(str word word)
213  pascal -ret16 vcpUICallbackProc(ptr word word long long) vcpUICallbackProc16
214  stub     VcpAddMRUPath #(str)
300  pascal -ret16 DiBuildCompatDrvList (ptr) DiBuildCompatDrvList16
301  pascal -ret16 DiBuildClassDrvList (ptr) DiBuildClassDrvList16
302  stub     DiDestroyDriverNodeList #(ptr)
303  pascal -ret16 DiCreateDeviceInfo (ptr str long long str str word) DiCreateDeviceInfo16
304  pascal -ret16 DiGetClassDevs(ptr str word word) DiGetClassDevs16
305  pascal -ret16 DiDestroyDeviceInfoList (ptr) DiDestroyDeviceInfoList16
306  stub     DiRemoveDevice #(ptr)
308  pascal -ret16 DiCallClassInstaller (word ptr) DiCallClassInstaller16
309  stub     DiCreateDriverNode #(ptr word word word str str str str str str long)
310  stub     DiDrawMiniIcon
311  stub     DiGetClassBitmapIndex #(str ptr)
312  stub     DiSelectDevice #(ptr)
313  stub     DiInstallDevice #(ptr)
314  stub     DiLoadClassIcon #(str ptr ptr)
315  stub     DiAskForOEMDisk #(ptr)
316  stub     Display_SetMode #(ptr word word word)
317  stub     Display_ClassInstaller #(word ptr)
318  pascal -ret16 DiCreateDevRegKey (ptr ptr word str word) DiCreateDevRegKey16
319  pascal -ret16 DiOpenDevRegKey (ptr ptr word) DiOpenDevRegKey16
320  stub     DiInstallDrvSection #(str str str str long)
321  stub     DiInstallClass #(str long)
322  stub     DiOpenClassRegKey #(ptr str)
323  stub     Display_SetFontSize #(str)
324  stub     Display_OpenFontSizeKey #(ptr)
325  stub     DiBuildClassDrvListFromOldInf #(ptr str ptr long)
326  stub     DiIsThereNeedToCopy #(word long)
333  stub     DiChangeState #(ptr long long long)
334  stub     WALKSUBTREE
340  stub     GetFctn #(word str str ptr ptr)
341  stub     DiBuildClassInfoList #(ptr)
342  stub     DiDestroyClassInfoList #(ptr)
343  stub     DiGetDeviceClassInfo #(ptr ptr)
344  pascal -ret16 DiDeleteDevRegKey (ptr word) DiDeleteDevRegKey16
350  stub     DiSelectOEMDrv #(word ptr)
351  stub     DiGetINFClass #(str word str long)
353  stub     DIPICKBESTDRIVER
355  stub     COPYINFFILE
360  stub     GenInfLCToDevNode #(word str  word word long)
361  stub     GETDOSMESSAGE
362  stub     Mouse_ClassInstaller #(word ptr)
363  stub     sxCompareDosAppVer #(str str)
364  stub     MONITOR_CLASSINSTALLER
365  stub     FCEGETRESDESOFFSET
366  stub     FCEGETALLOCVALUE
367  stub     FCEADDRESDES
368  stub     FCEDELETERESDES
369  stub     FCEINIT
370  stub     FCEGETRESDES
371  stub     FCEGETFIRSTVALUE
372  stub     FCEGETOTHERVALUE
373  stub     FCEGETVALIDATEVALUE
374  stub     FCEWRITETHISFORCEDCONFIGNOW
375  stub     SUCreatePropertySheetPage #(ptr)
376  stub     SUDestroyPropertySheetPage #(word)
377  stub     SUPropertySheet #(ptr)
380  stub     DiReadRegLogConf #(ptr str ptr ptr)
381  stub     DiReadRegConf #(ptr ptr ptr long)
390  stub     DiBuildPotentialDuplicatesList #(ptr ptr long ptr ptr)
395  stub     InitSubstrData #(ptr str)
396  stub     GetFirstSubstr #(ptr)
397  stub     GetNextSubstr #(ptr)
398  stub     INITSUBSTRDATAEX
400  stub     bIsFileInVMM32 #(str)
401  stub     DiInstallDriverFiles #(ptr)
405  stub     DiBuildClassInfoListEx #(ptr long)
406  stub     DiGetClassDevsEx #(ptr str str word word)
407  stub     DiCopyRegSubKeyValue #(word str str str)
408  stub     IPGETDRIVERDATE
409  stub     IPGETDRIVERVERSION
410  stub     IpGetVersionString #(str str ptr word str)
411  pascal   VcpExplain(ptr long) VcpExplain16
412  stub     DiBuildDriverIndex #(word)
413  stub     DiAddSingleInfToDrvIdx #(str word word)
414  stub     FCEGETFLAGS
450  stub     UiMakeDlgNonBold #(word)
451  stub     UiDeleteNonBoldFont #(word)
500  stub     SUEBDPAGE
501  stub     SUOCPAGE
502  stub     SXLISTSUBPROC
503  stub     SXFILLLB
504  stub     SXOCPAGEDLG
506  stub     SXOCBATCHSETTINGS
507  stub     SXOCFIXNEEDS
508  pascal -ret16 CtlSetLddPath(word str) CtlSetLddPath16
509  stub     SXCALLOCPROC
510  stub     BUILDINFOCS
511  stub     BUILDREGOCS
512  stub     DELETEOCS
520  stub     DiBuildClassDrvInfoList #(ptr)
521  stub     DiBuildCompatDrvInfoList #(ptr)
522  stub     DiDestroyDrvInfoList #(ptr)
523  stub     DiConvertDriverInfoToDriverNode #(ptr ptr)
524  stub     DISELECTBESTCOMPATDRV
525  stub     FirstBootMoveToDOSSTART #(str word)
526  stub     DOSOptEnableCurCfg #(str)
527  pascal -ret16 InstallHinfSection(word word str word) InstallHinfSection16
528  stub     SXMAKEUNCPATH
529  stub     SXISSBSSERVERFILE
530  stub     SXFINDBATCHFILES
531  stub     ISPANEUROPEAN
532  stub     UPGRADENIGGLINGS
533  stub     DISPLAY_ISSECONDDISPLAY
534  stub     ISWINDOWSFILE
540  stub     VERIFYSELECTEDDRIVER
575  stub     SXCALLMIGRATIONDLLS
576  stub     SXCALLMIGRATIONDLLS_RUNDLL
600  stub     PidConstruct #(str str str word)
601  stub     PidValidate #(str str)
602  stub     GETJAPANESEKEYBOARDTYPE
610  stub     CRC32COMPUTE
621  stub     SXSAVEINFO
622  stub     SXADDPAGEEX
623  stub     OPKREMOVEINSTALLEDNETDEVICE
640  stub     DOFIRSTRUNSCREENS
700  stub     SXSHOWREBOOTDLG
701  stub     SXSHOWREBOOTDLG_RUNDLL
750  stub     UIPOSITIONDIALOG
775  stub     ASPICLEAN
800  stub     EXTRACTCABFILE
825  stub     PIDGEN3
826  stub     GETSETUPINFO
827  stub     SETSETUPINFO
828  stub     GETKEYBOARDOPTIONS
829  stub     GETLOCALEOPTIONS
830  stub     SETINTLOPTIONS
831  stub     GETPRODUCTTYPE
832  stub     ISOPKMODE
833  stub     AUDITONETIMEINSTALL
834  stub     DISKDUP
835  stub     OPKPREINSTALL
836  stub     ISAUDITMODE
837  stub     ISAUDITAUTO
838  stub     GETVALIDEULA
