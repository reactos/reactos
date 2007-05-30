1      pascal  WEP(word word word ptr) MMSYSTEM_WEP
2      pascal  sndPlaySound(ptr word) sndPlaySound16
3      pascal  PlaySound(ptr word long) PlaySound16
4      pascal  DllEntryPoint(long word word word long word) MMSYSTEM_LibMain
5      pascal  mmsystemGetVersion() mmsystemGetVersion16
6      pascal  DriverProc(long word word long long) DriverProc16
8      pascal  WMMMidiRunOnce() WMMMidiRunOnce16
30     pascal -ret16 OutputDebugStr(str) OutputDebugStr16
31     pascal  DriverCallback(long word word word long long long) DriverCallback16
32     pascal  StackEnter() StackEnter16
33     pascal  StackLeave() StackLeave16
34     stub    MMDRVINSTALL
101    pascal  joyGetNumDevs() joyGetNumDevs16
102    pascal  joyGetDevCaps(word ptr word) joyGetDevCaps16
103    pascal  joyGetPos(word ptr) joyGetPos16
104    pascal  joyGetThreshold(word ptr) joyGetThreshold16
105    pascal  joyReleaseCapture(word) joyReleaseCapture16
106    pascal  joySetCapture(word word word word) joySetCapture16
107    pascal  joySetThreshold(word word) joySetThreshold16
109    pascal  joySetCalibration(word) joySetCalibration16
110    pascal  joyGetPosEx(word ptr) joyGetPosEx16
111    stub    JOYCONFIGCHANGED
201    pascal  midiOutGetNumDevs() midiOutGetNumDevs16
202    pascal  midiOutGetDevCaps(word ptr word) midiOutGetDevCaps16
203    pascal  midiOutGetErrorText(word ptr word) midiOutGetErrorText16
204    pascal  midiOutOpen(ptr word long long long) midiOutOpen16
205    pascal  midiOutClose(word) midiOutClose16
206    pascal  midiOutPrepareHeader(word segptr word) midiOutPrepareHeader16
207    pascal  midiOutUnprepareHeader(word segptr word) midiOutUnprepareHeader16
208    pascal  midiOutShortMsg(word long) midiOutShortMsg16
209    pascal  midiOutLongMsg(word segptr word) midiOutLongMsg16
210    pascal  midiOutReset(word) midiOutReset16
211    pascal  midiOutGetVolume(word ptr) midiOutGetVolume16
212    pascal  midiOutSetVolume(word long) midiOutSetVolume16
213    pascal  midiOutCachePatches(word word ptr word) midiOutCachePatches16
214    pascal  midiOutCacheDrumPatches(word word ptr word) midiOutCacheDrumPatches16
215    pascal  midiOutGetID(word ptr) midiOutGetID16
216    pascal  midiOutMessage(word word long long) midiOutMessage16
250    pascal  midiStreamProperty(word ptr long) midiStreamProperty16
251    pascal  midiStreamOpen(ptr ptr long long long long) midiStreamOpen16
252    pascal  midiStreamClose(word) midiStreamClose16
253    pascal  midiStreamPosition(word ptr word) midiStreamPosition16
254    pascal  midiStreamOut(word ptr word) midiStreamOut16
255    pascal  midiStreamPause(word) midiStreamPause16
256    pascal  midiStreamRestart(word) midiStreamRestart16
257    pascal  midiStreamStop(word) midiStreamStop16
301    pascal  midiInGetNumDevs() midiInGetNumDevs16
302    pascal  midiInGetDevCaps(word ptr word) midiInGetDevCaps16
303    pascal  midiInGetErrorText(word ptr word) midiOutGetErrorText16
304    pascal  midiInOpen(ptr word long long long) midiInOpen16
305    pascal  midiInClose(word) midiInClose16
306    pascal  midiInPrepareHeader(word segptr word) midiInPrepareHeader16
307    pascal  midiInUnprepareHeader(word segptr word) midiInUnprepareHeader16
308    pascal  midiInAddBuffer(word segptr word) midiInAddBuffer16
309    pascal  midiInStart(word) midiInStart16
310    pascal  midiInStop(word) midiInStop16
311    pascal  midiInReset(word) midiInReset16
312    pascal  midiInGetID(word ptr) midiInGetID16
313    pascal  midiInMessage(word word long long) midiInMessage16
350    pascal  auxGetNumDevs() auxGetNumDevs16
351    pascal  auxGetDevCaps(word ptr word) auxGetDevCaps16
352    pascal  auxGetVolume(word ptr) auxGetVolume16
353    pascal  auxSetVolume(word long) auxSetVolume16
354    pascal  auxOutMessage(word word long long) auxOutMessage16
401    pascal  waveOutGetNumDevs() waveOutGetNumDevs16
402    pascal  waveOutGetDevCaps(word ptr word) waveOutGetDevCaps16
403    pascal  waveOutGetErrorText(word ptr word) waveOutGetErrorText16
404    pascal  waveOutOpen(ptr word ptr long long long) waveOutOpen16
405    pascal  waveOutClose(word) waveOutClose16
406    pascal  waveOutPrepareHeader(word segptr word) waveOutPrepareHeader16
407    pascal  waveOutUnprepareHeader(word segptr word) waveOutUnprepareHeader16
408    pascal  waveOutWrite(word segptr word) waveOutWrite16
409    pascal  waveOutPause(word) waveOutPause16
410    pascal  waveOutRestart(word) waveOutRestart16
411    pascal  waveOutReset(word) waveOutReset16
412    pascal  waveOutGetPosition(word ptr word) waveOutGetPosition16
413    pascal  waveOutGetPitch(word ptr) waveOutGetPitch16
414    pascal  waveOutSetPitch(word long) waveOutSetPitch16
415    pascal  waveOutGetVolume(word ptr) waveOutGetVolume16
416    pascal  waveOutSetVolume(word long) waveOutSetVolume16
417    pascal  waveOutGetPlaybackRate(word ptr) waveOutGetPlaybackRate16
418    pascal  waveOutSetPlaybackRate(word long) waveOutSetPlaybackRate16
419    pascal  waveOutBreakLoop(word) waveOutBreakLoop16
420    pascal  waveOutGetID(word ptr) waveOutGetID16
421    pascal  waveOutMessage(word word long long) waveOutMessage16
501    pascal  waveInGetNumDevs() waveInGetNumDevs16
502    pascal  waveInGetDevCaps(word ptr word) waveInGetDevCaps16
503    pascal  waveInGetErrorText(word ptr word) waveOutGetErrorText16
504    pascal  waveInOpen(ptr word ptr long long long) waveInOpen16
505    pascal  waveInClose(word) waveInClose16
506    pascal  waveInPrepareHeader(word segptr word) waveInPrepareHeader16
507    pascal  waveInUnprepareHeader(word segptr word) waveInUnprepareHeader16
508    pascal  waveInAddBuffer(word segptr word) waveInAddBuffer16
509    pascal  waveInStart(word) waveInStart16
510    pascal  waveInStop(word) waveInStop16
511    pascal  waveInReset(word) waveInReset16
512    pascal  waveInGetPosition(word ptr word) waveInGetPosition16
513    pascal  waveInGetID(word ptr) waveInGetID16
514    pascal  waveInMessage(word word long long) waveInMessage16
601    pascal  timeGetSystemTime(ptr word) timeGetSystemTime16
602    pascal  timeSetEvent(word word segptr long word) timeSetEvent16
603    pascal  timeKillEvent(word) timeKillEvent16
604    pascal  timeGetDevCaps(ptr word) timeGetDevCaps16
605    pascal  timeBeginPeriod(word) timeBeginPeriod16
606    pascal  timeEndPeriod(word) timeEndPeriod16
607    pascal  timeGetTime() timeGetTime
701    pascal  mciSendCommand(word word long long) mciSendCommand16
702    pascal  mciSendString(str ptr word word) mciSendString16
703    pascal  mciGetDeviceID(ptr) mciGetDeviceID16
705    pascal  mciLoadCommandResource(word str word) mciLoadCommandResource16
706    pascal  mciGetErrorString(long ptr word) mciGetErrorString16
707    pascal  mciSetDriverData(word long) mciSetDriverData16
708    pascal  mciGetDriverData(word) mciGetDriverData16
710    pascal  mciDriverYield(word) mciDriverYield16
711    pascal  mciDriverNotify(word word word) mciDriverNotify16
712    pascal  mciExecute(ptr) mciExecute
713    pascal  mciFreeCommandResource(word) mciFreeCommandResource16
714    pascal  mciSetYieldProc(word ptr long) mciSetYieldProc16
715    pascal  mciGetDeviceIDFromElementID(long ptr) mciGetDeviceIDFromElementID16
716    pascal  mciGetYieldProc(word ptr) mciGetYieldProc16
717    pascal  mciGetCreatorTask(word) mciGetCreatorTask16
800    pascal  mixerGetNumDevs() mixerGetNumDevs16
801    pascal  mixerGetDevCaps(word ptr word) mixerGetDevCaps16
802    pascal  mixerOpen(ptr word long long long) mixerOpen16
803    pascal  mixerClose(word) mixerClose16
804    pascal  mixerMessage(word word long long) mixerMessage16
805    pascal  mixerGetLineInfo(word ptr long) mixerGetLineInfo16
806    pascal  mixerGetID(word ptr long) mixerGetID16
807    pascal  mixerGetLineControls(word ptr long) mixerGetLineControls16
808    pascal  mixerGetControlDetails(word ptr long) mixerGetControlDetails16
809    pascal  mixerSetControlDetails(word ptr long) mixerSetControlDetails16
900    pascal  mmTaskCreate(long ptr long) mmTaskCreate16
902    pascal  mmTaskBlock(word) mmTaskBlock16
903    pascal  mmTaskSignal(word) mmTaskSignal16
904    pascal -ret16 mmGetCurrentTask() mmGetCurrentTask16
905    pascal  mmTaskYield() mmTaskYield16
1100   pascal  DrvOpen(str str long) DrvOpen16
1101   pascal  DrvClose(word long long) DrvClose16
1102   pascal  DrvSendMessage(word word long long) DrvSendMessage16
1103   pascal  DrvGetModuleHandle(word) DrvGetModuleHandle16
1104   pascal  DrvDefDriverProc(long word word long long) DrvDefDriverProc16
1120   pascal  mmThreadCreate(segptr ptr long long) mmThreadCreate16
1121   pascal  mmThreadSignal(word) mmThreadSignal16
1122   pascal  mmThreadBlock(word) mmThreadBlock16
1123   pascal  mmThreadIsCurrent(word) mmThreadIsCurrent16
1124   pascal  mmThreadIsValid(word) mmThreadIsValid16
1125   pascal  mmThreadGetTask(word) mmThreadGetTask16
1150   pascal  mmShowMMCPLPropertySheet(word str str str) mmShowMMCPLPropertySheet16

1210   pascal  mmioOpen(str ptr long) mmioOpen16
1211   pascal  mmioClose(word word) mmioClose16
1212   pascal  mmioRead(word ptr long) mmioRead16
1213   pascal  mmioWrite(word ptr long) mmioWrite16
1214   pascal  mmioSeek(word long word) mmioSeek16
1215   pascal  mmioGetInfo(word ptr word) mmioGetInfo16
1216   pascal  mmioSetInfo(word ptr word) mmioSetInfo16
1217   pascal  mmioSetBuffer(word segptr long word) mmioSetBuffer16
1218   pascal  mmioFlush(word word) mmioFlush16
1219   pascal  mmioAdvance(word ptr word) mmioAdvance16
1220   pascal  mmioStringToFOURCC(str word) mmioStringToFOURCC16
1221   pascal  mmioInstallIOProc(long ptr long) mmioInstallIOProc16
1222   pascal  mmioSendMessage(word word long long) mmioSendMessage16
1223   pascal  mmioDescend(word ptr ptr word) mmioDescend16
1224   pascal  mmioAscend(word ptr word) mmioAscend16
1225   pascal  mmioCreateChunk(word ptr word) mmioCreateChunk16
1226   pascal  mmioRename(ptr ptr ptr long) mmioRename16

#2000   stub    WINMMF_THUNKDATA16
#2001   stub    RING3_DEVLOADER
#2002   stub    WINMMTILEBUFFER
#2003   stub    WINMMUNTILEBUFFER
#2005   stub    MCIGETTHUNKTABLE
#2006   stub    WINMMSL_THUNKDATA16

# these are Wine only exported functions. Is there another way to do it ?
2047   pascal  __wine_mmThreadEntryPoint(long) WINE_mmThreadEntryPoint
