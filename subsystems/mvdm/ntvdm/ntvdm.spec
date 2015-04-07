;;;;;;;;;;;;;;;;;;;;;
;; NTVDM Registers ;;
;;;;;;;;;;;;;;;;;;;;;

@ stdcall getAF()
@ stdcall getAH()
@ stdcall getAL()
@ stdcall getAX()
@ stdcall getBH()
@ stdcall getBL()
@ stdcall getBP()
@ stdcall getBX()
@ stdcall getCF()
@ stdcall getCH()
@ stdcall getCL()
@ stdcall getCS()
@ stdcall getCX()
@ stdcall getDF()
@ stdcall getDH()
@ stdcall getDI()
@ stdcall getDL()
@ stdcall getDS()
@ stdcall getDX()
@ stdcall getEAX()
@ stdcall getEBP()
@ stdcall getEBX()
@ stdcall getECX()
@ stdcall getEDI()
@ stdcall getEDX()
@ stdcall getEFLAGS()
@ stdcall getEIP()
@ stdcall getES()
@ stdcall getESI()
@ stdcall getESP()
@ stdcall getFS()
@ stdcall getGS()
@ stdcall getIF()
@ stdcall getIntelRegistersPointer()
@ stdcall getIP()
@ stdcall getMSW()
@ stdcall getOF()
@ stdcall getPF()
@ stdcall getSF()
@ stdcall getSI()
@ stdcall getSP()
@ stdcall getSS()
@ stdcall getZF()

@ stdcall setAF(long)
@ stdcall setAH(long)
@ stdcall setAL(long)
@ stdcall setAX(long)
@ stdcall setBH(long)
@ stdcall setBL(long)
@ stdcall setBP(long)
@ stdcall setBX(long)
@ stdcall setCF(long)
@ stdcall setCH(long)
@ stdcall setCL(long)
@ stdcall setCS(long)
@ stdcall setCX(long)
@ stdcall setDF(long)
@ stdcall setDH(long)
@ stdcall setDI(long)
@ stdcall setDL(long)
@ stdcall setDS(long)
@ stdcall setDX(long)
@ stdcall setEAX(long)
@ stdcall setEBP(long)
@ stdcall setEBX(long)
@ stdcall setECX(long)
@ stdcall setEDI(long)
@ stdcall setEDX(long)
@ stdcall setEFLAGS(long)
@ stdcall setEIP(long)
@ stdcall setES(long)
@ stdcall setESI(long)
@ stdcall setESP(long)
@ stdcall setFS(long)
@ stdcall setGS(long)
@ stdcall setIF(long)
@ stdcall setIP(long)
@ stdcall setMSW(long)
@ stdcall setOF(long)
@ stdcall setPF(long)
@ stdcall setSF(long)
@ stdcall setSI(long)
@ stdcall setSP(long)
@ stdcall setSS(long)
@ stdcall setZF(long)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; NTVDM CCPU MIPS Compatibility ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

@ stdcall c_getAF()  getAF
@ stdcall c_getAH()  getAH
@ stdcall c_getAL()  getAL
@ stdcall c_getAX()  getAX
@ stdcall c_getBH()  getBH
@ stdcall c_getBL()  getBL
@ stdcall c_getBP()  getBP
@ stdcall c_getBX()  getBX
@ stdcall c_getCF()  getCF
@ stdcall c_getCH()  getCH
@ stdcall c_getCL()  getCL
@ stdcall c_getCS()  getCS
@ stdcall c_getCX()  getCX
@ stdcall c_getDF()  getDF
@ stdcall c_getDH()  getDH
@ stdcall c_getDI()  getDI
@ stdcall c_getDL()  getDL
@ stdcall c_getDS()  getDS
@ stdcall c_getDX()  getDX
@ stdcall c_getEAX() getEAX
@ stdcall c_getEBP() getEBP
@ stdcall c_getEBX() getEBX
@ stdcall c_getECX() getECX
@ stdcall c_getEDI() getEDI
@ stdcall c_getEDX() getEDX
@ stdcall c_getEIP() getEIP
@ stdcall c_getES()  getES
@ stdcall c_getESI() getESI
@ stdcall c_getESP() getESP
@ stdcall c_getFS()  getFS
@ stdcall c_getGS()  getGS
@ stdcall c_getIF()  getIF
@ stdcall c_getIP()  getIP
@ stdcall c_getMSW() getMSW
@ stdcall c_getOF()  getOF
@ stdcall c_getPF()  getPF
@ stdcall c_getSF()  getSF
@ stdcall c_getSI()  getSI
@ stdcall c_getSP()  getSP
@ stdcall c_getSS()  getSS
@ stdcall c_getZF()  getZF

@ stdcall c_setAF(long)  setAF
@ stdcall c_setAH(long)  setAH
@ stdcall c_setAL(long)  setAL
@ stdcall c_setAX(long)  setAX
@ stdcall c_setBH(long)  setBH
@ stdcall c_setBL(long)  setBL
@ stdcall c_setBP(long)  setBP
@ stdcall c_setBX(long)  setBX
@ stdcall c_setCF(long)  setCF
@ stdcall c_setCH(long)  setCH
@ stdcall c_setCL(long)  setCL
@ stdcall c_setCS(long)  setCS
@ stdcall c_setCX(long)  setCX
@ stdcall c_setDF(long)  setDF
@ stdcall c_setDH(long)  setDH
@ stdcall c_setDI(long)  setDI
@ stdcall c_setDL(long)  setDL
@ stdcall c_setDS(long)  setDS
@ stdcall c_setDX(long)  setDX
@ stdcall c_setEAX(long) setEAX
@ stdcall c_setEBP(long) setEBP
@ stdcall c_setEBX(long) setEBX
@ stdcall c_setECX(long) setECX
@ stdcall c_setEDI(long) setEDI
@ stdcall c_setEDX(long) setEDX
@ stdcall c_setEIP(long) setEIP
@ stdcall c_setES(long)  setES
@ stdcall c_setESI(long) setESI
@ stdcall c_setESP(long) setESP
@ stdcall c_setFS(long)  setFS
@ stdcall c_setGS(long)  setGS
@ stdcall c_setIF(long)  setIF
@ stdcall c_setIP(long)  setIP
@ stdcall c_setMSW(long) setMSW
@ stdcall c_setOF(long)  setOF
@ stdcall c_setPF(long)  setPF
@ stdcall c_setSF(long)  setSF
@ stdcall c_setSI(long)  setSI
@ stdcall c_setSP(long)  setSP
@ stdcall c_setSS(long)  setSS
@ stdcall c_setZF(long)  setZF


;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; NTVDM DOS-32 Emulation ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;

@ stdcall demClientErrorEx(long long long)
@ stdcall demFileDelete(ptr)
@ stdcall demFileFindFirst(ptr ptr long)
@ stdcall demFileFindNext(ptr)
;@ stdcall demGetFileTimeByHandle_WOW
@ stdcall demGetPhysicalDriveType(long)
@ stdcall demIsShortPathName(ptr long)
;@ stdcall demLFNCleanup
;@ stdcall demLFNGetCurrentDirectory
@ stdcall demSetCurrentDirectoryGetDrive(ptr ptr)
;@ stdcall demWOWLFNAllocateSearchHandle
;@ stdcall demWOWLFNCloseSearchHandle
;@ stdcall demWOWLFNEntry
;@ stdcall demWOWLFNGetSearchHandle
;@ stdcall demWOWLFNInit


;;;;;;;;;;;;;;;;;;;;;;;;;
;; NTVDM Miscellaneous ;;
;;;;;;;;;;;;;;;;;;;;;;;;;

@ stdcall MGetVdmPointer(long long long)
@ stdcall Sim32pGetVDMPointer(long long)

;@ stdcall VdmFlushCache(long long long long) ; Not exported on x86
@ stdcall VdmMapFlat(long long long)
;@ stdcall VdmUnmapFlat(long long ptr long)  ; Not exported on x86

@ stdcall VDDInstallMemoryHook(long ptr long ptr)
@ stdcall VDDDeInstallMemoryHook(long ptr long)

@ stdcall VDDAllocMem(long ptr long)
@ stdcall VDDFreeMem(long ptr long)
@ stdcall VDDIncludeMem(long ptr long)
@ stdcall VDDExcludeMem(long ptr long)

@ stdcall call_ica_hw_interrupt(long long long)
@ stdcall VDDReserveIrqLine(long long)
@ stdcall VDDReleaseIrqLine(long long)

@ stdcall VDDInstallIOHook(long long ptr ptr)
@ stdcall VDDDeInstallIOHook(long long ptr)

@ stdcall VDDRequestDMA(long long ptr long)
@ stdcall VDDQueryDMA(long long ptr)
@ stdcall VDDSetDMA(long long long ptr)

@ stdcall VDDSimulate16()
@ stdcall host_simulate()   VDDSimulate16
@ stdcall VDDTerminateVDM()
