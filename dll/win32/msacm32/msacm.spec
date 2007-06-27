  1 stub     WEP
  2 stub     DRIVERPROC
  3 stub     ___EXPORTEDSTUB
  7 pascal   acmGetVersion() acmGetVersion16
  8 pascal -ret16 acmMetrics(word word ptr) acmMetrics16
 10 pascal -ret16 acmDriverEnum(ptr long long) acmDriverEnum16
 11 pascal -ret16 acmDriverDetails(word ptr long) acmDriverDetails16
 12 pascal -ret16 acmDriverAdd(ptr word long long long) acmDriverAdd16
 13 pascal -ret16 acmDriverRemove(word long) acmDriverRemove16
 14 pascal -ret16 acmDriverOpen(ptr word long) acmDriverOpen16
 15 pascal -ret16 acmDriverClose(word long) acmDriverClose16
 16 pascal   acmDriverMessage(word word long long) acmDriverMessage16
 17 pascal -ret16 acmDriverID(word ptr long) acmDriverID16
 18 pascal -ret16 acmDriverPriority(word long long) acmDriverPriority16
 30 pascal -ret16 acmFormatTagDetails(word ptr long) acmFormatTagDetails16
 31 pascal -ret16 acmFormatTagEnum(word ptr ptr long long) acmFormatTagEnum16
 40 pascal -ret16 acmFormatChoose(ptr) acmFormatChoose16
 41 pascal -ret16 acmFormatDetails(word ptr long) acmFormatDetails16
 42 pascal -ret16 acmFormatEnum(word ptr ptr long long) acmFormatEnum16
 45 pascal -ret16 acmFormatSuggest(word ptr ptr long long) acmFormatSuggest16
 50 pascal -ret16 acmFilterTagDetails(word ptr long) acmFilterTagDetails16
 51 pascal -ret16 acmFilterTagEnum(word ptr ptr long long) acmFilterTagEnum16
 60 pascal -ret16 acmFilterChoose(ptr) acmFilterChoose16
 61 pascal -ret16 acmFilterDetails(word ptr long) acmFilterDetails16
 62 pascal -ret16 acmFilterEnum(word ptr ptr long long) acmFilterEnum16
 70 pascal -ret16 acmStreamOpen(ptr word ptr ptr ptr long long long) acmStreamOpen16
 71 pascal -ret16 acmStreamClose(word long) acmStreamClose16
 72 pascal -ret16 acmStreamSize(word long ptr long) acmStreamSize16
 75 pascal -ret16 acmStreamConvert(word ptr long) acmStreamConvert16
 76 pascal -ret16 acmStreamReset(word long) acmStreamReset16
 77 pascal -ret16 acmStreamPrepareHeader(word ptr long) acmStreamPrepareHeader16
 78 pascal -ret16 acmStreamUnprepareHeader(word ptr long) acmStreamUnprepareHeader16
150 stub     ACMAPPLICATIONEXIT
175 stub     ACMHUGEPAGELOCK
176 stub     ACMHUGEPAGEUNLOCK
200 stub     ACMOPENCONVERSION
201 stub     ACMCLOSECONVERSION
202 stub     ACMCONVERT
203 stub     ACMCHOOSEFORMAT
255 pascal   DllEntryPoint(long word word word long word) MSACM_DllEntryPoint
