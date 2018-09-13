/** FILE: ports.c ********** Module Header ********************************
 *
 *  Class installer for the Ports class.
 *
 @@BEGIN_DDKSPLIT
 * History:
 *  12:30 on Tues  23 Apr 1991  -by-  Steve Cathcart   [stevecat]
 *        Took base code from Win 3.1 source
 *  10:30 on Tues  04 Feb 1992  -by-  Steve Cathcart   [stevecat]
 *        Updated code to latest Win 3.1 sources
 *  16:30 on Fri   27 Mar 1992  -by-  Steve Cathcart   [stevecat]
 *        Changed to allow for unlimited number of NT COM ports
 *  18:00 on Tue   06 Apr 1993  -by-  Steve Cathcart   [stevecat]
 *        Updated to work seamlessly with NT serial driver
 *  19:00 on Wed   05 Jan 1994  -by-  Steve Cathcart   [stevecat]
 *        Allow setting COM1 - COM4 advanced parameters
 @@END_DDKSPLIT
 *
 *  Copyright (C) 1990-1999 Microsoft Corporation
 *
 *************************************************************************/
//==========================================================================
//                                Include files
//==========================================================================
// C Runtime
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Application specific
#include "ports.h"
#include <msports.h>

// @@BEGIN_DDKSPLIT
#include <initguid.h>
//
// Instantiate GUID_NULL.
//
DEFINE_GUID(GUID_NULL, 0L, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
//
// Instantiate class installer GUIDs (interesting one is GUID_DEVCLASS_PORTS).
//
#include <devguid.h>
// @@END_DDKSPLIT

//==========================================================================
//                                Globals
//==========================================================================

HANDLE  g_hInst  = NULL;

TCHAR g_szClose[ 40 ];              //  "Close" string
TCHAR g_szErrMem[ 200 ];            //  Low memory message
TCHAR g_szPortsApplet[ 30 ];        //  "Ports Control Panel Applet" title
TCHAR g_szNull[]  = TEXT("");       //  Null string

TCHAR  m_szColon[]      = TEXT( ":" );
TCHAR  m_szComma[]      = TEXT( "," );
TCHAR  m_szCloseParen[] = TEXT( ")" );
TCHAR  m_szPorts[]      = TEXT( "Ports" );
TCHAR  m_szCOM[]        = TEXT( "COM" );
TCHAR  m_szSERIAL[]     = TEXT( "Serial" );
TCHAR  m_szLPT[]        = TEXT( "LPT" );

//
//  NT Registry keys to find COM port to Serial Device mapping
//
TCHAR m_szRegSerialMap[] = TEXT( "Hardware\\DeviceMap\\SerialComm" );
TCHAR m_szRegParallelMap[] = TEXT( "Hardware\\DeviceMap\\PARALLEL PORTS" );

//
//  Registry Serial Port Advanced I/O settings key and valuenames
//

TCHAR m_szRegServices[]  =
            TEXT( "System\\CurrentControlSet\\Services\\" );

TCHAR m_szRootEnumName[]         = REGSTR_KEY_ROOTENUM;
TCHAR m_szAcpiEnumName[]         = REGSTR_KEY_ACPIENUM;

TCHAR m_szFIFO[]                = TEXT( "ForceFifoEnable" );
TCHAR m_szDosDev[]              = TEXT( "DosDevices" );
TCHAR m_szPollingPeriod[]       = TEXT( "PollingPeriod" );
TCHAR m_szPortName[]            = REGSTR_VAL_PORTNAME;
TCHAR m_szDosDeviceName[]       = TEXT( "DosDeviceName" );
TCHAR m_szFirmwareIdentified[]  = TEXT( "FirmwareIdentified" );
TCHAR m_szPortSubClass[]         = REGSTR_VAL_PORTSUBCLASS;

// @@BEGIN_DDKSPLIT
//
// Strings needed for parallel port installation.
//
TCHAR m_szParallelClassDevName[] = TEXT( "Root\\ParallelClass\\0000" );
TCHAR m_szParallelClassHwId[]    = TEXT( "Root\\ParallelClass\0" );     // multi-sz
// @@END_DDKSPLIT

int m_nBaudRates[] = { 75, 110, 134, 150, 300, 600, 1200, 1800, 2400,
                       4800, 7200, 9600, 14400, 19200, 38400, 57600,
                       115200, 128000, 0 };

TCHAR m_sz9600[] = TEXT( "9600" );

TCHAR m_szDefParams[] = TEXT( "9600,n,8,1" );

short m_nDataBits[] = { 4, 5, 6, 7, 8, 0};

TCHAR *m_pszParitySuf[] = { TEXT( ",e" ),
                            TEXT( ",o" ),
                            TEXT( ",n" ),
                            TEXT( ",m" ),
                            TEXT( ",s" ) };

TCHAR *m_pszLenSuf[] = { TEXT( ",4" ),
                         TEXT( ",5" ),
                         TEXT( ",6" ),
                         TEXT( ",7" ),
                         TEXT( ",8" ) };

TCHAR *m_pszStopSuf[] = { TEXT( ",1" ),
                          TEXT( ",1.5" ),
                          TEXT( ",2 " ) };

TCHAR *m_pszFlowSuf[] = { TEXT( ",x" ),
                          TEXT( ",p" ),
                          TEXT( " " ) };

// @@BEGIN_DDKSPLIT
//
// Include the string-ified form of the Computer (i.e., HAL) class GUID here,
// so we don't have to pull in OLE or RPC just to get StringFromGuid.
//
TCHAR m_szComputerClassGuidString[] = TEXT( "{4D36E966-E325-11CE-BFC1-08002BE10318}" );

//
// String to append onto install section for COM ports in order to generate
// "PosDup" section.
//
TCHAR m_szPosDupSectionSuffix[] = (TEXT(".") INFSTR_SUBKEY_POSSIBLEDUPS);

//
// BUGBUG (lonnym)--may need to add NEC98 IDs to the following list (see
// comments in GetDetectedSerialPortsList).
//
TCHAR *m_pszSerialPnPIds[] = { TEXT( "*PNP0501" ) };

#define SERIAL_PNP_IDS_COUNT (sizeof(m_pszSerialPnPIds) / sizeof(m_pszSerialPnPIds[0]))
// @@END_DDKSPLIT

#define IN_RANGE(value, minval, maxval) ((minval) <= (value) && (value) <= (maxval))


//==========================================================================
//                            Local Function Prototypes
//==========================================================================

DWORD
InstallPnPSerialPort(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    );

// @@BEGIN_DDKSPLIT
DWORD
GetDetectedSerialPortsList(
    IN HDEVINFO DeviceInfoSet,
    IN BOOL     FirstTimeSetup
    );

DWORD
RegisterDetectedSerialPort(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    );

BOOL
GetPosDupList(
    IN  HDEVINFO           DeviceInfoSet, 
    IN  PSP_DEVINFO_DATA   DeviceInfoData,
    OUT PTSTR            **PosDupList, 
    OUT INT               *PosDupCount
    );
// @@END_DDKSPLIT

DWORD
InstallPnPParallelPort(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    );

DWORD
InstallSerialOrParallelPort(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    );

BOOL
IsDeviceParallelPort(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    );

// @@BEGIN_DDKSPLIT
BOOL
GetSerialPortDevInstConfig(
    IN  DEVINST            DevInst,
    IN  ULONG              LogConfigType,
    OUT PIO_RESOURCE       IoResource,             OPTIONAL
    OUT PIRQ_RESOURCE      IrqResource             OPTIONAL
    );
// @@END_DDKSPLIT



//==========================================================================
//                                Dll Entry Point
//==========================================================================
BOOL APIENTRY LibMain( HANDLE hDll, DWORD dwReason, LPVOID lpReserved )
{
    switch( dwReason )
    {
    case DLL_PROCESS_ATTACH:
        g_hInst = hDll;
        DisableThreadLibraryCalls(hDll);
        InitStrings();

        break;

    case DLL_PROCESS_DETACH:
        break;

    default:
        break;
    }

    return TRUE;
}


//==========================================================================
//                                Functions
//==========================================================================

DWORD
WINAPI
PortsClassInstaller(
    IN DI_FUNCTION      InstallFunction,
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
)
/*++

Routine Description:

    This routine acts as the class installer for Ports devices.

Arguments:

    InstallFunction - Specifies the device installer function code indicating
        the action being performed.

    DeviceInfoSet - Supplies a handle to the device information set being
        acted upon by this install action.

    DeviceInfoData - Optionally, supplies the address of a device information
        element being acted upon by this install action.

Return Value:

    If this function successfully completed the requested action, the return
        value is NO_ERROR.

    If the default behavior is to be performed for the requested action, the
        return value is ERROR_DI_DO_DEFAULT.

    If an error occurred while attempting to perform the requested action, a
        Win32 error code is returned.

--*/
{
    SP_MOVEDEV_PARAMS   MoveDevParams;
    SP_INSTALLWIZARD_DATA   iwd;
    HKEY                hDeviceKey;
    HCOMDB              hComDB;
    DWORD               PortNameSize, 
                        Err,
                        size;
    TCHAR               PortName[20];
    BOOL                result;

    switch(InstallFunction) {

        case DIF_INSTALLDEVICE :

            return InstallSerialOrParallelPort(DeviceInfoSet, DeviceInfoData);

        case DIF_MOVEDEVICE :
            //
            // In addition to doing the default action of calling
            // SetupDiMoveDuplicateDevice, we need to retrieve the COM port
            // number of the old device, and set it to be the COM port number
            // of the new device (if the move is successful).  In Win95, setupx
            // had a hack inside of DiMoveDuplicateDevNode to do that, but it's
            // really the class installer's job to do class-specific stuff like
            // this.
            //
            MoveDevParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
            if(!SetupDiGetClassInstallParams(DeviceInfoSet,
                                             DeviceInfoData,
                                             (PSP_CLASSINSTALL_HEADER)&MoveDevParams,
                                             sizeof(MoveDevParams),
                                             NULL)) {
                return GetLastError();
            }

            //
            // Open the device key for the source device instance, and retrieve its
            // "PortName" value.
            //
            if((hDeviceKey = SetupDiOpenDevRegKey(DeviceInfoSet,
                                                  &MoveDevParams.SourceDeviceInfoData,
                                                  DICS_FLAG_GLOBAL,
                                                  0,
                                                  DIREG_DEV,
                                                  KEY_READ)) == INVALID_HANDLE_VALUE) {
                return GetLastError();
            }

            PortNameSize = sizeof(PortName);
            Err = RegQueryValueEx(hDeviceKey,
                                  m_szPortName,
                                  NULL,
                                  NULL,
                                  (PBYTE)PortName,
                                  &PortNameSize
                                 );

            RegCloseKey(hDeviceKey);

            if(Err != ERROR_SUCCESS) {
                return Err;
            }

            //
            // Now call the default routine for moving devices.
            //
            if(!SetupDiMoveDuplicateDevice(DeviceInfoSet, DeviceInfoData)) {
                return GetLastError();
            }

            //
            // The device was successfully moved.  Now open the destination device's key,
            // and store the COM port name there.  (Ignore failure here.)
            //
            if((hDeviceKey = SetupDiOpenDevRegKey(DeviceInfoSet,
                                                  &MoveDevParams.SourceDeviceInfoData,
                                                  DICS_FLAG_GLOBAL,
                                                  0,
                                                  DIREG_DEV,
                                                  KEY_READ)) != INVALID_HANDLE_VALUE) {

                RegSetValueEx(hDeviceKey,
                              m_szPortName,
                              0,
                              REG_SZ,
                              (PBYTE)PortName,
                              ByteCountOf(lstrlen(PortName) + 1)
                             );

                RegCloseKey(hDeviceKey);
            }

            return NO_ERROR;

        case DIF_REMOVE:
            
            if (!IsParPort(DeviceInfoSet, DeviceInfoData)) {
                if (ComDBOpen(&hComDB) == ERROR_SUCCESS) {

                    hDeviceKey = SetupDiOpenDevRegKey(DeviceInfoSet,
                                                      DeviceInfoData,
                                                      DICS_FLAG_GLOBAL,
                                                      0,
                                                      DIREG_DEV,
                                                      KEY_READ);

                    if (hDeviceKey !=   INVALID_HANDLE_VALUE) {
                        PortNameSize = sizeof(PortName);
                        Err = RegQueryValueEx(hDeviceKey,
                                              m_szPortName,
                                              NULL,
                                              NULL,
                                              (PBYTE)PortName,
                                              &PortNameSize
                                             );
                        RegCloseKey(hDeviceKey);

                        if (Err == ERROR_SUCCESS) {
                            ComDBReleasePort(hComDB,
                                             myatoi(PortName+wcslen(m_szCOM)));
                        }
                    }

                    ComDBClose(hComDB);
                }
            }

            if (!SetupDiRemoveDevice(DeviceInfoSet, DeviceInfoData)) {
                return GetLastError();
            }

            return NO_ERROR;

        // @@BEGIN_DDKSPLIT
        case DIF_FIRSTTIMESETUP:
        case DIF_DETECT:

            return GetDetectedSerialPortsList(DeviceInfoSet,
                                              (InstallFunction == DIF_FIRSTTIMESETUP)
                                             );

        case DIF_REGISTERDEVICE:

            return RegisterDetectedSerialPort(DeviceInfoSet,
                                              DeviceInfoData
                                             );
        // @@END_DDKSPLIT

        default :
            //
            // Just do the default action.
            //
            return ERROR_DI_DO_DEFAULT;
    }
}


DWORD
InstallSerialOrParallelPort(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    )
/*++

Routine Description:

    This routine installs either a serial or a parallel port.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        the device being installed.

    DeviceInfoData - Supplies the address of the device information element
        being installed.

Return Value:

    If successful, the return value is NO_ERROR, otherwise it is a Win32 error code.

--*/
{
    if(IsDeviceParallelPort(DeviceInfoSet, DeviceInfoData)) {
        return InstallPnPParallelPort(DeviceInfoSet, DeviceInfoData);
    } else {
        return InstallPnPSerialPort(DeviceInfoSet, DeviceInfoData);
    }
}
    
BOOL
IsDeviceParallelPort(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    )
/*++

Routine Description:

    This routine determines whether the driver node selected for the specified device
    is for a parallel (LPT or ECP) or serial (COM) port.  It knows which is which by
    running the AddReg entries in the driver node's install section, and then looking
    in the devnode's driver key for a 'PortSubClass' value entry.  If this value is
    present, and set to 0, then this is an LPT or ECP port, otherwise we treat it like
    a COM port.  This value was relied upon in Win9x, so it is the safest way for us
    to make this determination.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        the device being installed.

    DeviceInfoData - Supplies the address of the device information element
        being installed.

Return Value:

    If the device is an LPT or ECP port, the return value is nonzero, otherwise it is
    FALSE.  (If anything goes wrong, the default is to return FALSE.)

--*/
{
    SP_DRVINFO_DATA DriverInfoData;
    SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;
    HINF hInf;
    HKEY hkDrv;
    TCHAR ActualInfSection[LINE_LEN];
    DWORD RegDataType;
    BYTE RegData;
    DWORD RegDataSize;

    //
    // Retrieve information about the driver node selected for this device.
    //
    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if(!SetupDiGetSelectedDriver(DeviceInfoSet, DeviceInfoData, &DriverInfoData)) {
        return FALSE;
    }

    DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
    if(!SetupDiGetDriverInfoDetail(DeviceInfoSet,
                                   DeviceInfoData,
                                   &DriverInfoData,
                                   &DriverInfoDetailData,
                                   sizeof(DriverInfoDetailData),
                                   NULL)
       && (GetLastError() != ERROR_INSUFFICIENT_BUFFER)) {
        //
        // For some reason we couldn't get detail data--this should never happen.
        //
        return FALSE;
    }

    //
    // Open the INF that installs this driver node, so we can 'pre-run' the AddReg
    // entries in its install section.
    //
    hInf = SetupOpenInfFile(DriverInfoDetailData.InfFileName,
                            NULL,
                            INF_STYLE_WIN4,
                            NULL
                           );
    if(hInf == INVALID_HANDLE_VALUE) {
        //
        // For some reason we couldn't open the INF--this should never happen.
        //
        return FALSE;
    }

    //
    // Open up the driver key for this device so we can run our INF registry mods
    // against it.
    //
    hkDrv = SetupDiCreateDevRegKey(DeviceInfoSet,
                                   DeviceInfoData,
                                   DICS_FLAG_GLOBAL,
                                   0,
                                   DIREG_DRV,
                                   NULL,
                                   NULL
                                  );
    if (hkDrv == INVALID_HANDLE_VALUE) {
        //
        // We couldn't create the driver key--this should never happen.
        //
        SetupCloseInfFile(hInf);
        return FALSE;
    }

    //
    // Now find the actual (potentially OS/platform-specific) install section name.
    //
    SetupDiGetActualSectionToInstall(hInf,
                                     DriverInfoDetailData.SectionName,
                                     ActualInfSection,
                                     sizeof(ActualInfSection) / sizeof(TCHAR),
                                     NULL,
                                     NULL
                                    );

    //
    // Now run the registry modification (AddReg/DelReg) entries in this section...
    //
    SetupInstallFromInfSection(NULL,    // no UI, so don't need to specify window handle
                               hInf,
                               ActualInfSection,
                               SPINST_REGISTRY,
                               hkDrv,
                               NULL,
                               0,
                               NULL,
                               NULL,
                               NULL,
                               NULL
                              );

    SetupCloseInfFile(hInf);

    //
    // Check for a REG_BINARY (1-byte) 'PortSubClass' value entry set to 0.
    //
    RegDataSize = sizeof(RegData);
    if((ERROR_SUCCESS != RegQueryValueEx(hkDrv,
                                         m_szPortSubClass,
                                         NULL,
                                         &RegDataType,
                                         &RegData,
                                         &RegDataSize))
       || (RegDataSize != sizeof(BYTE))
       || (RegDataType != REG_BINARY))
    {
        RegData = 1; // not a LPT/ECP device.
    }

    RegCloseKey(hkDrv);

    return !RegData;
}
    
// @@BEGIN_DDKSPLIT
//
// If the preferred value is available, let them have that one
//
VOID
GenerateLptNumber(PDWORD Num,
                  DWORD  PreferredValue)
{
    HKEY    parallelMap;
    TCHAR   valueName[40];
    TCHAR   lptName[60], *lptNameLocation;

    int     i = 0;
    DWORD   valueSize, lptSize, regDataType, newLptNum;
    DWORD   highestLptNum = 0;
    BOOL    change = FALSE;
    TCHAR   errorMsg[MAX_PATH];

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     m_szRegParallelMap,
                     0,
                     KEY_QUERY_VALUE ,
                     &parallelMap) != ERROR_SUCCESS) {
        return;
    }


    valueSize = sizeof(valueName);
    lptSize = sizeof(lptName);
    while (ERROR_SUCCESS == RegEnumValue(parallelMap,
                                         i++,
                                         valueName,
                                         &valueSize,
                                         NULL,
                                         &regDataType,
                                         (LPBYTE) lptName,
                                         &lptSize)) {
        if (regDataType == REG_SZ) {
            lptNameLocation = wcsstr(_wcsupr(lptName), m_szLPT);
            if (lptNameLocation) {
                newLptNum = myatoi(lptNameLocation + wcslen(m_szLPT));
                if (newLptNum == PreferredValue) {
                    change = TRUE;
                }
                if (newLptNum > highestLptNum) {
                    highestLptNum = newLptNum;
                }
            }
        }

        valueSize = sizeof(valueName);
        lptSize = sizeof(lptName);
    }
    
    if (change) {
        *Num = highestLptNum + 1;
    } else {
        *Num = PreferredValue;
    }

    RegCloseKey(parallelMap);
}

BOOL
DetermineLptNumberFromResources(
    IN  DEVINST            DevInst,
    OUT PDWORD             Num
    )
/*++

Routine Description:

    This routine retrieves the base IO port and IRQ for the specified device instance
    in a particular logconfig.

Arguments:

    DevInst - Supplies the handle of a device instance to retrieve configuration for.

Return Value:

    If success, the return value is TRUE, otherwise it is FALSE.

--*/
{
    LOG_CONF    logConfig;
    RES_DES     resDes;
    CONFIGRET   cr;
    BOOL        success;
    IO_RESOURCE ioResource;
    WORD        base;
    ULONGLONG base2;

    if (CM_Get_First_Log_Conf(&logConfig,
                              DevInst,
                              BOOT_LOG_CONF) != CR_SUCCESS) {
        GenerateLptNumber(Num, 1);
        return TRUE;
    }

    success = FALSE;    // assume failure.

    //
    // First, get the Io base port
    //
    if (CM_Get_Next_Res_Des(&resDes,
                            logConfig,
                            ResType_IO,
                            NULL,
                            0) != CR_SUCCESS) {
        goto clean0;
    }

    cr = CM_Get_Res_Des_Data(resDes,
                             &ioResource,
                             sizeof(IO_RESOURCE),
                             0);

    CM_Free_Res_Des_Handle(resDes);

    if (cr != CR_SUCCESS) {
        goto clean0;
    }

    success = TRUE;


    //
    // Values for resources from ISA Architecture
    //

    base = (WORD) ioResource.IO_Header.IOD_Alloc_Base;

    if (IN_RANGE(base, 0x278, 0x27f)) {
        *Num = 2;
    }
    else if (IN_RANGE(base, 0x378, 0x37f)) {
        *Num = 1;
    }
    else if (base == 0x3bc) {
        *Num = 1;
    }
    else {
        //
        // Most machines only have one port anways, so just try that here
        //
        GenerateLptNumber(Num, 1);
    }

clean0:
    CM_Free_Log_Conf_Handle(logConfig);

    return success;
}
// @@END_DDKSPLIT

DWORD
InstallPnPParallelPort(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    )
/*++

// @@BEGIN_DDKSPLIT
Routine Description:

    This routine installs a parallel (LPT or ECP) port.  If there is no
    root-enumerated devnode installed for the parallel class driver (parallel.sys),
    we will create one and install it right on the spot.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        the device being installed.

    DeviceInfoData - Supplies the address of the device information element
        being installed.

Return Value:

    If successful, the return value is NO_ERROR, otherwise it is a Win32 error code.
    
    //
    // IGNORE the decription below, it is for the DDK only
    //
    
// @@END_DDKSPLIT

Routine Description:

    This routine installs a parallel port. In the DDK implementation, we let the
    default setup installer run and do nothing special.
    
--*/
{
// @@BEGIN_DDKSPLIT
    HDEVINFO        parClassDevInfoSet;
    SP_DEVINFO_DATA parClassDevInfoData;
    SP_DEVINSTALL_PARAMS parClassDevInstallParams;
    SP_DEVINSTALL_PARAMS parClassDevInstallParamsIn;
    TCHAR           charBuffer[LINE_LEN],
                    friendlyNameFormat[LINE_LEN],
                    deviceDesc[LINE_LEN],
                    lptPortName[20];
    PTCHAR          lptLocation;
    DWORD           lptPortNameSize, lptNum;
    HKEY            hDeviceKey;
    TCHAR           lpszService[MAX_PATH];
    DWORD           error;

    //
    // We init the value here so that the DDK version of the function we have an
    // initialized value when it returns err.  In the shipping version of this 
    // function, we immediately set this to a different value.
    //
// @@END_DDKSPLIT

    DWORD           err = ERROR_DI_DO_DEFAULT;

// @@BEGIN_DDKSPLIT
    err = ERROR_SUCCESS;

    //
    // Predispose the port name to 1.  On almost any machine imaginable, there
    // will only be ONE LPT port, so we might as well assume it.
    //
    lptNum = 1;

    ZeroMemory(lptPortName, sizeof(lptPortName));

    //
    // First, make sure that Device Parameters\PortName exists and contains a 
    // valid value so that when the parallel driver starts, it can name the
    // device
    //

    if ((hDeviceKey = SetupDiCreateDevRegKey(DeviceInfoSet,
                                             DeviceInfoData,
                                             DICS_FLAG_GLOBAL,
                                             0,
                                             DIREG_DEV,
                                             NULL,
                                             NULL)) != INVALID_HANDLE_VALUE) {
        //
        // Retrieve the port name.
        //
        lptPortNameSize = sizeof(lptPortName);
        if (RegQueryValueEx(hDeviceKey,
                            m_szPortName,
                            NULL,
                            NULL,
                            (PBYTE)lptPortName,
                            &lptPortNameSize) != ERROR_SUCCESS) {
            lptPortNameSize = sizeof(lptPortName); 
            if (RegQueryValueEx(hDeviceKey,
                                m_szDosDeviceName,
                                NULL,
                                NULL,
                                (PBYTE) lptPortName,
                                &lptPortNameSize) != ERROR_SUCCESS) {

                if (SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                                     DeviceInfoData,
                                                     SPDRP_ENUMERATOR_NAME,
                                                     NULL,
                                                     (PBYTE)charBuffer,
                                                     sizeof(charBuffer),
                                                     NULL)) {

                    if (lstrcmpi(charBuffer, m_szAcpiEnumName) == 0) {
                        wsprintf(lptPortName, _T("%s%d"), m_szLPT, 1);
                    }
                }

                if (*lptPortName != _T('\0')) {
                    DWORD dwSize, dwFirmwareIdentified;

                    dwSize = sizeof(dwFirmwareIdentified);
                    if (RegQueryValueEx(hDeviceKey,
                                        m_szFirmwareIdentified,
                                        NULL,
                                        NULL,
                                        (PBYTE) &dwFirmwareIdentified,
                                        &dwSize) == ERROR_SUCCESS) {
                        //
                        // ACPI puts the value "FirmwareIdentified" if it has enumerated 
                        // this port.  We only rely on this if a DDN isn't present and we
                        // couldn't get the enumerator name
                        //
                        wsprintf(lptPortName, _T("%s%d"), m_szLPT, 1);
                    }
                }
            }
        }

        if (lptPortName[0] != (TCHAR) 0) {

            _wcsupr(lptPortName);
            lptLocation = wcsstr(lptPortName, m_szLPT);
            if (lptLocation) {
                lptNum = myatoi(lptLocation + wcslen(m_szLPT));
            } else {
                DetermineLptNumberFromResources((DEVINST) DeviceInfoData->DevInst,
                                                &lptNum);
            }
        }
        else {
            DetermineLptNumberFromResources((DEVINST) DeviceInfoData->DevInst,
                                            &lptNum);
        }
        //
        // Check if this is a brand new port by querying the service value.
        // On a newly detected port, there will be no service value.
        //
        if (!SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                             DeviceInfoData,
                                             SPDRP_SERVICE,
                                             NULL,
                                             (LPBYTE) lpszService,
                                             MAX_PATH*sizeof(TCHAR),
                                             NULL)) {
            if (13L == GetLastError())
            {
                GenerateLptNumber(&lptNum, lptNum);
            }
        }

        wsprintf(lptPortName, _T("LPT%d"), lptNum);

        //
        // If this fails, then we can't do much about it but continue
        //
        RegSetValueEx(hDeviceKey,
                      m_szPortName,
                      0,
                      REG_SZ,
                      (PBYTE) lptPortName,
                      ByteCountOf(lstrlen(lptPortName) + 1)
                      );
        
        RegCloseKey(hDeviceKey);
    }

    //
    // Second, let the default installation take place.
    //
    if (!SetupDiInstallDevice(DeviceInfoSet, DeviceInfoData)) {
        return GetLastError();
    }

    //
    // Now generate a string, to be used for the device's friendly name, that incorporates
    // both the INF-specified device description, and the port name.  For example,
    //
    //     ECP Printer Port (LPT1)
    //

    if (LoadString(g_hInst, 
                   IDS_FRIENDLY_FORMAT, 
                   friendlyNameFormat, 
                   CharSizeOf(friendlyNameFormat)) &&
       SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                        DeviceInfoData,
                                        SPDRP_DEVICEDESC,
                                        NULL,
                                        (PBYTE)deviceDesc,
                                        sizeof(deviceDesc),
                                        NULL)) {
        wsprintf(charBuffer, friendlyNameFormat, deviceDesc, lptPortName);
    }
    else {
        //
        // Simply use LPT port name.
        //
        lstrcpy(charBuffer, lptPortName);
    }

    SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                     DeviceInfoData,
                                     SPDRP_FRIENDLYNAME,
                                     (PBYTE)charBuffer,
                                     ByteCountOf(lstrlen(charBuffer) + 1)
                                    );

    //
    // Create a device information set to contain the (existing or newly-created)
    // parallel class devnode.
    //
    parClassDevInfoSet = SetupDiCreateDeviceInfoList(NULL, NULL);
    if (parClassDevInfoSet == INVALID_HANDLE_VALUE) {
        //
        // We must be out of memory.
        //
        return GetLastError();
    }

    //
    // Now, create a "Root\ParallelClass\0000" devnode (this will fail if there's
    // already one.
    //
    parClassDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    if (!SetupDiCreateDeviceInfo(parClassDevInfoSet,
                                 m_szParallelClassDevName,
                                 (LPGUID)&GUID_NULL,
                                 NULL,
                                 NULL,
                                 0,
                                 &parClassDevInfoData)) {
        err = GetLastError();

        //
        // Destroy the device info set before handling the error.
        //
        SetupDiDestroyDeviceInfoList(parClassDevInfoSet);

        if (err == ERROR_DEVINST_ALREADY_EXISTS) {
            //
            // The device already exists--we're done.
            //
            return NO_ERROR;
        } else {
            //
            // We couldn't create the device for some other reason.
            //
            return err;
        }
    }

    //
    // Assume success from this point forward.
    //
    err = NO_ERROR;

    //
    // We successfully created the root-enumerated device, so now we need to install it.
    // Write out a HardwareID of "ParallelClass", that will match up with a driver node in
    // our parallel class INF.
    //
    if(!SetupDiSetDeviceRegistryProperty(parClassDevInfoSet,
                                         &parClassDevInfoData,
                                         SPDRP_HARDWAREID,
                                         (PBYTE)m_szParallelClassHwId,
                                         sizeof(m_szParallelClassHwId))) {
        err = GetLastError();
        goto clean0;
    }

    //
    // Now build a compatible driver list for this device.
    //
    if(!SetupDiBuildDriverInfoList(parClassDevInfoSet,
                                   &parClassDevInfoData,
                                   SPDIT_COMPATDRIVER)) {
        err = GetLastError();
        goto clean0;
    }

    //
    // Now select the best driver (there'll typically only be one).
    //
    if(!SetupDiCallClassInstaller(DIF_SELECTBESTCOMPATDRV,
                                  parClassDevInfoSet,
                                  &parClassDevInfoData)) {
        err = GetLastError();
        goto clean0;
    }

    //
    // Register this device.
    //
    if(!SetupDiCallClassInstaller(DIF_REGISTERDEVICE,
                                  parClassDevInfoSet,
                                  &parClassDevInfoData)) {
        err = GetLastError();
        goto clean0;
    }

    //
    // Verify that it's OK to install this guy (this should always succeed!)
    //
    if(!SetupDiCallClassInstaller(DIF_ALLOW_INSTALL,
                                  parClassDevInfoSet,
                                  &parClassDevInfoData)) {
        //
        // The error had better be do-default...
        //
        err = GetLastError();

        if(err == ERROR_DI_DO_DEFAULT) {
            //
            // This isn't an error--we may proceed with the installation.
            //
            err = NO_ERROR;
        } else {
            //
            // Some yahoo has vetoed the install.  Gotta bail...
            //
            goto clean0;
        }
    }

    //
    // OK, now we're ready to install this device.  Note that we can't do any copying
    // at this point.  Thus, any INF that wants to install (replace) our parallel class
    // support must conform to the rule that the driver node for the underlying port
    // installs any files needed for (a) the parallel class device, (b) any co-installers
    // for (a), and (c) any device interface files for (a).
    //
    // Retrieve the device install params and turn off file copying.
    // Also copy flags that should be mirrored from DeviceInfoSet
    //
    parClassDevInstallParamsIn.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    parClassDevInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    //
    // get flags from DevInfoSet we came in with
    //
    if(!SetupDiGetDeviceInstallParams(DeviceInfoSet,
                                     DeviceInfoData,
                                     &parClassDevInstallParamsIn)) {
        err = GetLastError();
        goto clean0;
    }

    if(SetupDiGetDeviceInstallParams(parClassDevInfoSet,
                                     &parClassDevInfoData,
                                     &parClassDevInstallParams)) {

        parClassDevInstallParams.Flags |= DI_NOFILECOPY;
        //
        // mirror the following flags in the new device's install parameters to
        // match those of the underlying parallel port:
        //
        // DI_QUIETINSTALL -- this install should always be quiet (we don't
        //                    copy any files!), but if the caller explicitly
        //                    requested this, then make sure that it is.
        //
        // DI_FLAGSEX_IN_SYSTEM_SETUP -- not doing this can cause RunOnce to 
        //                               execute when it shouldn't
        //
        parClassDevInstallParams.Flags |= (parClassDevInstallParamsIn.Flags & DI_QUIETINSTALL);
        parClassDevInstallParams.FlagsEx |= (parClassDevInstallParamsIn.FlagsEx & DI_FLAGSEX_IN_SYSTEM_SETUP);

        SetupDiSetDeviceInstallParams(parClassDevInfoSet,
                                      &parClassDevInfoData,
                                      &parClassDevInstallParams
                                     );
    }

    if(!SetupDiCallClassInstaller(DIF_REGISTER_COINSTALLERS,
                                  parClassDevInfoSet,
                                  &parClassDevInfoData)) {
        err = GetLastError();
        goto clean0;
    }


    //
    // OK, now finish up the installation...
    //
    if(!SetupDiCallClassInstaller(DIF_INSTALLINTERFACES,
                                  parClassDevInfoSet,
                                  &parClassDevInfoData)) {
        err = GetLastError();
        goto clean0;
    }

    if(!SetupDiCallClassInstaller(DIF_INSTALLDEVICE,
                                  parClassDevInfoSet,
                                  &parClassDevInfoData)) {
        err = GetLastError();
        goto clean0;
    }

clean0:

    if (err != NO_ERROR) {
        //
        // Clean up the devnode we were trying to install.
        //
        SetupDiCallClassInstaller(DIF_REMOVE, parClassDevInfoSet, &parClassDevInfoData);
    }

    SetupDiDestroyDeviceInfoList(parClassDevInfoSet);

    //
    // Ignore the comments below, but KEEP THEM IN.  We need them for the DDK
    //
// @@END_DDKSPLIT

    //
    // Let the default setup installer install parallel ports for the DDK
    // version of this class installer
    //
    return err;
}

// @@BEGIN_DDKSPLIT
DWORD
GetDetectedSerialPortsList(
    IN HDEVINFO DeviceInfoSet,
    IN BOOL     FirstTimeSetup
    )
/*++

Routine Description:

    This routine retrieves a list of all root-enumerated COM port device
    instances that are not manually installed (both phantoms and non-phantoms),
    and adds those device instances to the supplied device information set.
                                                                     
    BUGBUG (lonnym)--There are a whole raft of device IDs that may exist on
    NEC98 systems (from ntos\io\mapper.c):                  
                                                                     
      *nEC1500                                                       
      *nEC1501                                                       
      *nEC1502                                                       
      *nEC1503                                                       
      *nEC8071                                                       
      *nEC0C01                                                       
                                                                     
    Should we include these (i.e., will our resource comparison logic work for
    these)?  The code in ntos\io\pnpmap.c!PnPBiosEliminateDupes that eliminates           
    duplicates between ntdetect and PnPBIOS doesn't seem to make any 
    distinction.                                                     
    
Arguments:

    DeviceInfoSet - Supplies a handle to the device information set into which
    the detected serial port elements are to be added.

    FirstTimeSetup - If non-zero, then we're in GUI-mode setup (responding to
        DIF_FIRSTTIMESETUP), and we only want to report (unregistered) devnodes
        created by the firmware mapper.
    
Return Value:

    If successful, the return value is NO_ERROR, otherwise it is a Win32 error
    code indicating the cause of failure.

--*/
{
    CONFIGRET cr;
    PTCHAR DevIdBuffer;
    ULONG DevIdBufferLen, Status, Problem;
    PTSTR CurDevId, DeviceIdPart, p;
    DWORD i;
    DEVNODE DevNode;
    HWND hwndParent = NULL;
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    SP_DEVINFO_DATA DeviceInfoData;

    //
    // First retrieve a list of all root-enumerated device instances.
    //
    while(TRUE) {
        
        cr = CM_Get_Device_ID_List_Size(&DevIdBufferLen,
                                        m_szRootEnumName,
                                        CM_GETIDLIST_FILTER_ENUMERATOR
                                       );

        if((cr != CR_SUCCESS) || !DevIdBufferLen) {
            //
            // This should never happen.
            //
            return ERROR_INVALID_DATA;
        }

        if(!(DevIdBuffer = LocalAlloc(LPTR, DevIdBufferLen * sizeof(TCHAR)))) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        cr = CM_Get_Device_ID_List(m_szRootEnumName,
                                   DevIdBuffer,
                                   DevIdBufferLen,
                                   CM_GETIDLIST_FILTER_ENUMERATOR
                                  );

        if(cr == CR_SUCCESS) {
            //
            // Device list retrieved successfully.
            //
            break;

        } else {
            //
            // Free the current buffer before determining what error occurred.
            //
            LocalFree(DevIdBuffer);

            //
            // If the error we encountered was anything other than buffer-too-
            // small, then we have to bail.  (Note: since we sized our buffer
            // up-front, the only time we'll hit buffer-too-small is if someone
            // else is creating root-enumerated devnodes while we're trying to
            // retrieve the list.)
            //
            if(cr != CR_BUFFER_SMALL) {
                return ERROR_INVALID_DATA;
            }
        }
    }

    //
    // Retrieve the HWND associated with the device information set, so we can
    // specify that same handle for any device information elements we create.
    //
    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    if(SetupDiGetDeviceInstallParams(DeviceInfoSet, NULL, &DeviceInstallParams)) {
        hwndParent = DeviceInstallParams.hwndParent;
    }

    //
    // Now examine each device ID in our list, looking for ones that match up
    // with the list of serial IDs that the firmware mapper can report.
    //
    for(CurDevId = DevIdBuffer;
        *CurDevId;
        CurDevId += lstrlen(CurDevId) + 1) {

        //
        // Skip over the root-enumerator prefix plus the first backslash.
        //
        DeviceIdPart = CurDevId + (sizeof(m_szRootEnumName) / sizeof(TCHAR));

        //
        // Find the next backslash and temporarily replace it with a NULL char.
        //
        p = _tcschr(DeviceIdPart, TEXT('\\'));

        *p = TEXT('\0');

        for(i = 0; i < SERIAL_PNP_IDS_COUNT; i++) {

            if(!lstrcmpi(DeviceIdPart, m_pszSerialPnPIds[i])) {
                //
                // We found a match
                //
                break;
            }
        }

        //
        // Before checking to see if we found a match, restore the backslash
        //
        *p = TEXT('\\');

        if(i >= SERIAL_PNP_IDS_COUNT) {
            //
            // We don't care about this device instance--move on to the next 
            // one.
            //
            continue;
        }

        //
        // Next, attempt to locate the devnode (either present or not-present).
        // Note that this call _will not_ succeed for device instances that are
        // "private phantoms" (i.e., marked with the "Phantom" flag in their
        // device instance key by the firmware mapper or by some other process
        // that has created a new root-enumerated device instance, but has not
        // yet registered it).
        //
        cr = CM_Locate_DevNode(&DevNode,
                               CurDevId,
                               CM_LOCATE_DEVINST_PHANTOM
                              );

        if(cr == CR_SUCCESS) {
            //
            // We are dealing with a device that has been registered.  It may
            // or may not be present, however.  Attempt to retrieve its status.
            // If that fails, the device isn't present, and we don't want to 
            // return it in our list of detected serial ports.  Also, we want
            // to skip this device if it was manually installed.
            //
            // Also, make sure we're processing DIF_DETECT.  We don't want to 
            // do this for DIF_FIRSTTIMESETUP, because GUI-mode setup doesn't 
            // pay attention to what previously-detected devices are no longer
            // found, so all we end up doing is causing two installs for each
            // detected device.
            //
            if(FirstTimeSetup 
               || (CR_SUCCESS != CM_Get_DevNode_Status(&Status,
                                                       &Problem,
                                                       DevNode,
                                                       0))
               || (Status & DN_MANUAL)) {

                //
                // Move on to the next device.
                //
                continue;
            }

            //
            // OK, now we can add this device information element to our set of
            // detected devices.  Regardless of success or failure, we're done
            // with this device--it's time to move on to the next one.
            //
            SetupDiOpenDeviceInfo(DeviceInfoSet, 
                                  CurDevId,
                                  hwndParent,
                                  0,
                                  NULL
                                 );
            continue;
        }

        //
        // If we get to here, then we've found a private phantom.  Create a
        // device information element for this device.  The underlying code
        // that implements CM_Create_DevInst won't allow creation of a device
        // instance that's already a private phantom _unless_ that device
        // instance was created by the firmware mapper.  Thus, we don't have to
        // worry about the (admittedly unlikely) case that we caught a private
        // phantom created by someone else (e.g., another detection in
        // progress.)
        //
        DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        if(!SetupDiCreateDeviceInfo(DeviceInfoSet,
                                    CurDevId,
                                    &GUID_DEVCLASS_PORTS,
                                    NULL,
                                    hwndParent,
                                    0,
                                    &DeviceInfoData)) {
            //
            // We were unable to create a device information element for this
            // private phantom (maybe because it wasn't a creation of the
            // firmware mapper).  At any rate, there's nothing we can do, so
            // skip this device and continue on.
            //
            continue;
        }

        //
        // OK, we have a device information element for our detected serial
        // port.  DIF_FIRSTTIMESETUP expects us to have a driver selected for
        // any devices we return.  DIF_DETECT doesn't make this requirement,
        // but it does respect the driver selection, if we make one.  Thus, we
        // always go ahead and do the compatible driver search ourselves.
        //
        if(!SetupDiBuildDriverInfoList(DeviceInfoSet,
                                       &DeviceInfoData,
                                       SPDIT_COMPATDRIVER)) {
            //
            // This should never fail--if it does, bail and move on to the next
            // device.
            //
            SetupDiDeleteDeviceInfo(DeviceInfoSet, &DeviceInfoData);
            continue;
        }

        //
        // Now select the best driver from among the compatible matches for the
        // device.
        //
        if(!SetupDiCallClassInstaller(DIF_SELECTBESTCOMPATDRV,
                                      DeviceInfoSet,
                                      &DeviceInfoData)) {
            //
            // This shouldn't fail, unless something really bad has happened
            // such as the user deleting %windir%\Inf\msports.inf.  If that
            // happens, then once again we've no choice but to bail and move on
            // to the next device.
            //
            SetupDiDeleteDeviceInfo(DeviceInfoSet, &DeviceInfoData);
            continue;
        }

        //
        // We've successfully added the detected device to the device info set.
        // On to the next device...
        //
    }

    LocalFree(DevIdBuffer);

    return NO_ERROR;
}


DWORD
RegisterDetectedSerialPort(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    )
/*++

Routine Description:

    This routine performs duplicate detection on the specified device
    information element, and if it isn't found to be a duplicate of any
    existing device, this devinfo element is registered (thus transforming it
    from just registry spooge into a real, live root-enumerated devnode).
    
Arguments:

    DeviceInfoSet - Supplies the handle of the device information set that
        contains the element to be registered.
        
    DeviceInfoData - Supplies the context structure for the device information
        element to be registered.

Return Value:

    If the device isn't a duplicate, the return value is NO_ERROR.
    Otherwise, it is some other Win32 error code indicating the cause of
    failure.  The most common failure is due to having detected the device as
    being a duplicate of an existing one--in that case the error reported is 
    ERROR_DUPLICATE_FOUND.
    
Remarks:

    If the device being registered wasn't created via device detection (i.e.,
    it doesn't have a boot config), then we just return ERROR_DI_DO_DEFAULT.

--*/
{
    CONFIGRET cr;
    LOG_CONF LogConf;
    RES_DES ResDes;
    IO_RESOURCE IoResource;
    CONFLICT_LIST ConflictList;
    ULONG ConflictCount, ConflictIndex;
    CONFLICT_DETAILS ConflictDetails;
    INT i, PosDupIndex, PosDupCount;
    PTCHAR IdBuffer = NULL;
    ULONG IdBufferSize = 0;
    DWORD Err;
    PCTSTR p;
    PTCHAR SerialDevNodeList = NULL;
    ULONG SerialDevNodeListSize;
    TCHAR CharBuffer[MAX_DEVNODE_ID_LEN];
    ULONG CharBufferSize;
    PTSTR *PosDupList;

    //
    // First, check to see if the boot config for this device conflicts with
    // any other device.  If it doesn't, then we know we don't have a duplicate.
    //
    if(!GetSerialPortDevInstConfig((DEVNODE)(DeviceInfoData->DevInst),
                                   BOOT_LOG_CONF,
                                   &IoResource,
                                   NULL)) {
        //
        // The device instance doesn't have a boot config--this will happen if
        // the user is attempting to manually install a COM port (i.e., not via
        // detection).  In this case, just let the default behavior happen.
        //
        return ERROR_DI_DO_DEFAULT;
    }

    //
    // We can't query for the resource conflict list on a phantom devnode.
    // Therefore, we are forced to register this devnode now, then uninstall it
    // later if we discover that it is, in fact, a duplicate.
    //
    if(!SetupDiRegisterDeviceInfo(DeviceInfoSet,
                                  DeviceInfoData,
                                  0,
                                  NULL,
                                  NULL,
                                  NULL)) {
        //
        // Device couldn't be registered.
        //
        return GetLastError();
    }

    cr = CM_Query_Resource_Conflict_List(&ConflictList,
                                         (DEVNODE)(DeviceInfoData->DevInst),
                                         ResType_IO,
                                         &IoResource,
                                         sizeof(IoResource),
                                         0,
                                         NULL
                                        );

    if(cr != CR_SUCCESS) {
        //
        // Couldn't retrieve a conflict list--assume there are no conflicts,
        // thus this device isn't a duplicate.
        //
        return NO_ERROR;
    }

    //
    // Find out how many things conflicted.
    //
    if((CR_SUCCESS != CM_Get_Resource_Conflict_Count(ConflictList, &ConflictCount))
       || !ConflictCount) {

        //
        // Either we couldn't retrieve the conflict count, or it was zero.  In
        // any case, we should assume this device isn't a duplicate.
        //
        Err = NO_ERROR;
        goto clean1;
    }

    //
    // Retrieve the list of devnodes with which the Serial service is
    // associated (as either the function driver or a filter driver).
    //
    SerialDevNodeListSize = 1024; // start out with a 1K character buffer

    while(TRUE) {

        if(!(SerialDevNodeList = LocalAlloc(LPTR, SerialDevNodeListSize))) {
            //
            // Out of memory--time to bail!
            //
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean1;
        }

        cr = CM_Get_Device_ID_List(m_szSERIAL,
                                   SerialDevNodeList,
                                   SerialDevNodeListSize,
                                   CM_GETIDLIST_FILTER_SERVICE
                                  );

        if(cr == CR_SUCCESS) {
            break;
        }

        LocalFree(SerialDevNodeList);
        SerialDevNodeList = NULL;

        if(cr != CR_BUFFER_SMALL) {
            //
            // We failed for some reason other than buffer-too-small.  Maybe
            // the Serial service isn't even installed.  At any rate, we'll
            // just skip this part of our check when processing the conflicting
            // devnodes below.
            //
            break;
        }

        //
        // Figure out how big of a buffer we actually need, 
        //
        cr = CM_Get_Device_ID_List_Size(&SerialDevNodeListSize,
                                        m_szSERIAL,
                                        CM_GETIDLIST_FILTER_SERVICE
                                       );
        if(cr != CR_SUCCESS) {
            //
            // This shouldn't fail, but if it does we'll just do without the
            // list.
            //
            break;
        }
    }

    //
    // Retrieve the list of possible duplicate IDs
    //
    if(!GetPosDupList(DeviceInfoSet, DeviceInfoData, &PosDupList, &PosDupCount)) {
        //
        // We couldn't retrieve the PosDup list for some reason--default to
        // the list of IDs known to be spat out by the firmware mapper.
        //
        PosDupList = m_pszSerialPnPIds;
        PosDupCount = SERIAL_PNP_IDS_COUNT;
    }

    //
    // Loop through each conflict, checking to see whether our device is a
    // duplicate of any of them.
    //
    for(ConflictIndex = 0; ConflictIndex < ConflictCount; ConflictIndex++) {

        ZeroMemory(&ConflictDetails, sizeof(ConflictDetails));

        ConflictDetails.CD_ulSize = sizeof(CONFLICT_DETAILS);
        ConflictDetails.CD_ulMask = CM_CDMASK_DEVINST | CM_CDMASK_FLAGS;

        cr = CM_Get_Resource_Conflict_Details(ConflictList,
                                              ConflictIndex,
                                              &ConflictDetails
                                             );

        //
        // If we failed to retrieve the conflict details, or if the conflict
        // was not with a PnP devnode, then we can ignore this conflict.
        //
        if((cr != CR_SUCCESS)
           || (ConflictDetails.CD_dnDevInst == -1)
           || (ConflictDetails.CD_ulFlags & (CM_CDFLAGS_DRIVER 
                                             | CM_CDFLAGS_ROOT_OWNED
                                             | CM_CDFLAGS_RESERVED))) {
            continue;    
        }

        //
        // We have a devnode--first check to see if this is the HAL devnode
        // (class = "Computer").  If so, then we've found the serial port in
        // use by the kernel debugger.
        //
        CharBufferSize = sizeof(CharBuffer);
        cr = CM_Get_DevNode_Registry_Property(ConflictDetails.CD_dnDevInst,
                                              CM_DRP_CLASSGUID,
                                              NULL,
                                              CharBuffer,
                                              &CharBufferSize,
                                              0
                                             );
        
        if((cr == CR_SUCCESS) && !lstrcmpi(CharBuffer, m_szComputerClassGuidString)) {
            //
            // We're conflicting with the HAL, presumably because it's claimed
            // the serial port IO addresses for use as the kernel debugger port.
            //
            // There are 3 scenarios:
            //
            //   1. non-ACPI, non-PnPBIOS machine -- detection is not required
            //      on these machines, because the mapper-reported devnodes are
            //      not reported as phantoms in the first place.
            //
            //   2. PnPBIOS or ACPI machine, debugger on PnP COM port -- we 
            //      don't want to install our detected devnode because it's a 
            //      duplicate.
            //
            //   3. PnPBIOS or ACPI machine, debugger on legacy COM port -- we
            //      _should_ install this devnode, because otherwise having the
            //      kernel debugger hooked up will prevent us from detecting
            //      the COM port.
            //
            // Unfortunately, we can't distinguish between cases (2) and (3) on
            // ACPI machines, because ACPI doesn't enumerate a devnode for the
            // serial port that's being used as the kernel debugger.  For now,
            // we're going to punt case (3) and say "tough"--you have to
            // disable the kernel debugger, reboot and re-run the hardware
            // wizard.  This isn't too bad considering that it's no worse than
            // what would happen if we actually had to poke at ports to detect
            // the COM port.  In that case, too, we would be unable to detect
            // the COM port if it was already in use by the debugger.
            //
            Err = ERROR_DUPLICATE_FOUND;
            goto clean2;
        }
        
        //
        // OK, we're not looking at the kernel debugger port.  Now check to see
        // if one of our known mapper-reported IDs is among this device's list
        // of hardware or compatible IDs.
        //
        for(i = 0; i < 2; i++) {

            cr = CM_Get_DevNode_Registry_Property(ConflictDetails.CD_dnDevInst,
                                                  (i ? CM_DRP_COMPATIBLEIDS 
                                                     : CM_DRP_HARDWAREID),
                                                  NULL,
                                                  IdBuffer,
                                                  &IdBufferSize,
                                                  0
                                                 );

            if(cr == CR_BUFFER_SMALL) {

                if(IdBuffer) {
                    LocalFree(IdBuffer);
                }

                if(!(IdBuffer = LocalAlloc(LPTR, IdBufferSize))) {
                    //
                    // Out of memory--time to bail!
                    //
                    Err = ERROR_NOT_ENOUGH_MEMORY;
                    goto clean2;
                }

                //
                // Decrement our index, so when we loop around again, we'll
                // re-attempt to retrieve the same property.
                //
                i--;
                continue;

            } else if(cr != CR_SUCCESS) {
                //
                // Failed to retrieve the property--just move on to the next
                // one.
                //
                continue;
            }

            //
            // If we get to here, we successfully retrieved a multi-sz list of
            // hardware or compatible IDs for this device.
            //
            for(p = IdBuffer; *p; p += (lstrlen(p) + 1)) {
                for(PosDupIndex = 0; PosDupIndex < PosDupCount; PosDupIndex++) {
                    if(!lstrcmpi(p, PosDupList[PosDupIndex])) {
                        //
                        // We found a match--our guy's a dupe.
                        //
                        Err = ERROR_DUPLICATE_FOUND;
                        goto clean2;
                    }
                }
            }
        }

        //
        // If we get to here, then we didn't find any duplicates based on ID
        // matching.  However, there are some 16550-compatible PnP devices that
        // don't report the correct compatible ID.  However, we have another
        // trick we can use--if the device has serial.sys as either the
        // function driver or a filter driver, then this is a solid indicator
        // that we have a dupe.
        //
        if(SerialDevNodeList) {
            //
            // Retrieve the name of this devnode so we can compare it against
            // the list of devnodes with which the Serial service is associated.
            //
            if(CR_SUCCESS == CM_Get_Device_ID(ConflictDetails.CD_dnDevInst,
                                              CharBuffer,
                                              sizeof(CharBuffer) / sizeof(TCHAR),
                                              0)) {

                for(p = SerialDevNodeList; *p; p += (lstrlen(p) + 1)) {
                    if(!lstrcmpi(CharBuffer, p)) {
                        //
                        // This devnode is using serial.sys--it must be a dupe.
                        //
                        Err = ERROR_DUPLICATE_FOUND;
                        goto clean2;
                    }
                }
            }
        }
    }

    //
    // If we get here, then all our checks have past--our newly-detected device
    // instance is not a duplicate of any other existing devnodes.
    //
    Err = NO_ERROR;

clean2:
    if(SerialDevNodeList) {
        LocalFree(SerialDevNodeList);
    }
    if(IdBuffer) {
        LocalFree(IdBuffer);
    }
    if(PosDupList != m_pszSerialPnPIds) {
        for(PosDupIndex = 0; PosDupIndex < PosDupCount; PosDupIndex++) {
            LocalFree(PosDupList[PosDupIndex]);
        }
        LocalFree(PosDupList);
    }

clean1:
    CM_Free_Resource_Conflict_Handle(ConflictList);

    if(Err != NO_ERROR) {
        //
        // Since we registered the devnode, we must manually uninstall it if
        // we fail.
        //
        SetupDiRemoveDevice(DeviceInfoSet, DeviceInfoData);
    }

    return Err;
}


BOOL
GetPosDupList(
    IN  HDEVINFO           DeviceInfoSet, 
    IN  PSP_DEVINFO_DATA   DeviceInfoData,
    OUT PTSTR            **PosDupList, 
    OUT INT               *PosDupCount
    )
/*++

Routine Description:

    This routine retrieves the list of PosDup IDs contained in the
    [<ActualInstallSec>.PosDup] INF section for the device information 
    element's selected driver node.
    
Arguments:

    DeviceInfoSet - Supplies the handle of the device information set that
        contains the device information element for which a driver is selected
        
    DeviceInfoData - Supplies the context structure for the device information
        element for which a driver node is selected.  The PosDup list will be
        retrieved based on this driver node's (potentially decorated) INF
        install section.

    PosDupList - Supplies the address of a pointer that will be set, upon
        successful return, to point to a newly-allocated array of string
        pointers, each pointing to a newly-allocated string buffer containing
        a device ID referenced in the relevant PosDup section for the selected
        driver node.
    
    PosDupCount - Supplies the address of an integer variable that, upon
        successful return, receives the number of string pointers stored in the
        PosDupList array.

Return Value:

    If successful, the return value is non-zero.  The caller is responsible for
    freeing each string pointer in the array, as well as the array buffer 
    itself.
    
    If unsuccessful, the return value is zero (FALSE).  (Note: the call is also
    considered unsuccessful if there's no associated PosDup section, or if it's
    empty).

--*/
{
    SP_DRVINFO_DATA DriverInfoData;
    SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;
    HINF hInf;
    TCHAR InfSectionWithExt[255];   // MAX_SECT_NAME_LEN from setupapi\inf.h
    BOOL b = FALSE;
    LONG LineCount, LineIndex;
    INFCONTEXT InfContext;
    DWORD NumElements, NumFields, FieldIndex;
    TCHAR PosDupId[MAX_DEVICE_ID_LEN];
    PTSTR PosDupCopy;

    //
    // Get the driver node selected for the specified device information
    // element.
    //
    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if(!SetupDiGetSelectedDriver(DeviceInfoSet, DeviceInfoData, &DriverInfoData)) {
        //
        // No driver node selected--there's nothing we can do!
        //
        goto clean0;
    }

    //
    // Now retrieve the corresponding INF and install section.
    //
    DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
    if(!SetupDiGetDriverInfoDetail(DeviceInfoSet,
                                   DeviceInfoData,
                                   &DriverInfoData,
                                   &DriverInfoDetailData,
                                   sizeof(DriverInfoDetailData),
                                   NULL)
       && (GetLastError() != ERROR_INSUFFICIENT_BUFFER)) {
        //
        // We failed, and it wasn't because the buffer was too small.  We gotta
        // bail.
        //
        goto clean0;
    }

    //
    // Open the INF for this driver node.
    //
    hInf = SetupOpenInfFile(DriverInfoDetailData.InfFileName, 
                            NULL, 
                            INF_STYLE_WIN4, 
                            NULL
                           );

    if(hInf == INVALID_HANDLE_VALUE) {
        goto clean0;
    }

    //
    // Get the (potentially decorated) install section name.
    //
    if(!SetupDiGetActualSectionToInstall(hInf,
                                         DriverInfoDetailData.SectionName,
                                         InfSectionWithExt,
                                         sizeof(InfSectionWithExt) / sizeof(TCHAR),
                                         NULL,
                                         NULL)) {
        goto clean1;
    }

    //
    // Append ".PosDup" to decorated install section.
    //
    lstrcat(InfSectionWithExt, m_szPosDupSectionSuffix);

    //
    // First, figure out the size of the array we're going to populate...
    //
    NumElements = 0;

    //
    // Loop through each line in the PosDup section.
    //
    LineCount = SetupGetLineCount(hInf, InfSectionWithExt);

    for(LineIndex = 0; LineIndex < LineCount; LineIndex++) {
        if(SetupGetLineByIndex(hInf, InfSectionWithExt, LineIndex, &InfContext)) {
            NumElements += SetupGetFieldCount(&InfContext);
        }
    }

    if(!NumElements) {
        //
        // We didn't find any PosDup entries.
        //
        goto clean1;
    }

    //
    // Now allocate a buffer big enough to hold all these entries.
    //
    *PosDupList = LocalAlloc(LPTR, NumElements * sizeof(PTSTR));

    if(!*PosDupList) {
        goto clean1;
    }

    *PosDupCount = 0;

    //
    // Now loop though each PosDup entry, and store copies of those entries in
    // our array.
    //
    for(LineIndex = 0; LineIndex < LineCount; LineIndex++) {

        if(SetupGetLineByIndex(hInf, InfSectionWithExt, LineIndex, &InfContext)) {

            NumFields = SetupGetFieldCount(&InfContext);

            for(FieldIndex = 1; FieldIndex <= NumFields; FieldIndex++) {

                if(!SetupGetStringField(&InfContext, 
                                        FieldIndex,
                                        PosDupId,
                                        sizeof(PosDupId) / sizeof(TCHAR),
                                        NULL)) {
                    //
                    // This shouldn't fail, but if it does, just move on to the
                    // next field.
                    //
                    continue;
                }

                PosDupCopy = LocalAlloc(LPTR,
                                        (lstrlen(PosDupId) + 1) * sizeof(TCHAR)
                                       );
                if(!PosDupCopy) {
                    goto clean2;
                }

                lstrcpy(PosDupCopy, PosDupId);

                (*PosDupList)[(*PosDupCount)++] = PosDupCopy;
            }
        }
    }

    //
    // If we get to here, and we found even one PosDup entry, consider the
    // operation a success
    //
    if(*PosDupCount) {
        b = TRUE;
        goto clean1;
    }

clean2:
    //
    // Something bad happened--clean up all memory allocated.
    //
    {
        INT i;

        for(i = 0; i < *PosDupCount; i++) {
            LocalFree((*PosDupList)[i]);
        }
        LocalFree(*PosDupList);
    }

clean1:
    SetupCloseInfFile(hInf);

clean0:
    return b;
}


// @@END_DDKSPLIT

#define NO_COM_NUMBER 0

BOOL
DetermineComNumberFromResources(
    IN  DEVINST            DevInst,
    OUT PDWORD             Num
    )
/*++

Routine Description:

    This routine retrieves the base IO port and IRQ for the specified device instance
    in a particular logconfig.

    If a successful match is found, then *Num == found number, otherwise
    *Num == NO_COM_NUMBER.
    
Arguments:

    DevInst - Supplies the handle of a device instance to retrieve configuration for.

Return Value:

    If success, the return value is TRUE, otherwise it is FALSE.

--*/
{
    LOG_CONF    logConfig;
    RES_DES     resDes;
    CONFIGRET   cr;
    BOOL        success;
    IO_RESOURCE ioResource;
    WORD        base;
    ULONGLONG base2;

    success = FALSE;    // assume failure.
    *Num = NO_COM_NUMBER;

    //
    // If the device does not have a boot config, use the com db
    //
    if (CM_Get_First_Log_Conf(&logConfig,
                              DevInst,
                              BOOT_LOG_CONF) != CR_SUCCESS) {
        return success;
    }

    //
    // First, get the Io base port
    //
    if (CM_Get_Next_Res_Des(&resDes,
                            logConfig,
                            ResType_IO,
                            NULL,
                            0) != CR_SUCCESS) {
        goto clean0;
    }

    cr = CM_Get_Res_Des_Data(resDes,
                             &ioResource,
                             sizeof(IO_RESOURCE),
                             0);

    CM_Free_Res_Des_Handle(resDes);

    if (cr != CR_SUCCESS) {
        goto clean0;
    }

    //
    // Values for resources from ISA Architecture
    //
    base = (WORD) ioResource.IO_Header.IOD_Alloc_Base;
    if (IN_RANGE(base, 0x3f8, 0x3ff)) {
        *Num = 1;
    }
    else if (IN_RANGE(base, 0x2f8, 0x2ff)) {
        *Num = 2;
    }
    else if (IN_RANGE(base, 0x3e8, 0x3ef)) {
        *Num = 3;
    }
    else if (IN_RANGE(base, 0x2e8, 0x2ef)) {
        *Num = 4;
    }

    if (*Num != NO_COM_NUMBER) {
        success = TRUE;
    }

clean0:
    CM_Free_Log_Conf_Handle(logConfig);

    return success;
}

#define DEF_MIN_COM_NUM (5)

DWORD
InstallPnPSerialPort(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    )
/*++

Routine Description:

    This routine performs the installation of a PnP ISA serial port device (may
    actually be a modem card).  This involves the following steps:

        1.  Select a COM port number and serial device name for this port
            (This involves duplicate detection, since PnP ISA cards will
            sometimes have a boot config, and thus be reported by ntdetect/ARC
            firmware.)
        2.  Create a subkey under the serial driver's Parameters key, and
            set it up just as if it was a manually-installed port.
        3.  Display the resource selection dialog, and allow the user to
            configure the settings for the port.
        4.  Write out the settings to the serial port's key in legacy format
            (i.e., the way serial.sys expects to see it).
        5.  Write out PnPDeviceId value to the serial port's key, which gives
            the device instance name with which this port is associated.
        6.  Write out PortName value to the devnode key, so that modem class
            installer can continue with installation (if this is really a
            PnP ISA modem).

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set containing
        the device being installed.

    DeviceInfoData - Supplies the address of the device information element
        being installed.

Return Value:

    If successful, the return value is NO_ERROR, otherwise it is a Win32 error code.

--*/
{
    HKEY        hKey; 
    HCOMDB      hComDB;
    TCHAR       comPort[40],
                szPortName[20],
                charBuffer[MAX_PATH],
                friendlyNameFormat[LINE_LEN],
                deviceDesc[LINE_LEN];
    PTCHAR      comLocation;
    DWORD       comPortSize, 
                comPortNumber = NO_COM_NUMBER,
                portsReported;
    DWORD       dwFirmwareIdentified, dwSize;
    BYTE        portUsage[32];
    BOOL        res;
    DWORD       firmwarePort = FALSE;

#if MAX_DEVICE_ID_LEN > MAX_PATH
#error MAX_DEVICE_ID_LEN is greater than MAX_PATH.  Update charBuffer.
#endif

    ZeroMemory(comPort, sizeof(comPort));

    ComDBOpen(&hComDB);
    
    if ((hKey = SetupDiOpenDevRegKey(DeviceInfoSet,
                                     DeviceInfoData,
                                     DICS_FLAG_GLOBAL,
                                     0,
                                     DIREG_DEV,
                                     KEY_READ)) != INVALID_HANDLE_VALUE) {
        
        comPortSize = sizeof(comPort);
        if (RegQueryValueEx(hKey,
                            m_szPortName,
                            NULL,
                            NULL,
                            (PBYTE)comPort,
                            &comPortSize) == ERROR_SUCCESS) {
            firmwarePort = TRUE;
        }
        else if ((comPortSize = sizeof(comPort)) &&
                 RegQueryValueEx(hKey,
                                 m_szDosDeviceName,
                                 NULL,
                                 NULL,
                                 (PBYTE) comPort,
                                 &comPortSize) == ERROR_SUCCESS) {
            //
            // ACPI puts the name of the port as DosDeviceName, use this name
            // as the basis for what to call this port
            //
            firmwarePort = TRUE;
        }
        else {
            //
            // Our final check is to check the enumerator.  We care about two 
            // cases:
            // 
            // 1)  If the enumerators is ACPI.  If so, blindly consider this 
            //     a firmware port (and get the BIOS mfg to provide a _DDN method
            //     for this device!)
            //
            // 2)  The port is "root" enumerated, yet it's not marked as
            // DN_ROOT_ENUMERATED.  This is the
            // way we distinguish PnPBIOS-reported devnodes.  Note that, in
            // general, these devnodes would've been caught by the check for a
            // "PortName" value above, but this won't be present if we couldn't
            // find a matching ntdetect-reported device from which to migrate
            // the COM port name.
            //
            // Note also that this check doesn't catch ntdetect or firmware
            // reported devices.  In these cases, we should already have a 
            // PortName, thus the check above should catch those devices.  In
            // the unlikely event that we encounter an ntdetect or firmware 
            // devnode that doesn't already have a COM port name, then it'll 
            // get an arbitrary one assigned.  Oh well.
            //
            if (SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                                 DeviceInfoData,
                                                 SPDRP_ENUMERATOR_NAME,
                                                 NULL,
                                                 (PBYTE)charBuffer,
                                                 sizeof(charBuffer),
                                                 NULL)) {
                if (lstrcmpi(charBuffer, m_szAcpiEnumName) == 0) {
                    firmwarePort = TRUE;
                }
                else if (lstrcmpi(charBuffer, m_szRootEnumName) == 0) {
                    ULONG status, problem;
    
                    if ((CM_Get_DevNode_Status(&status, 
                                               &problem, 
                                               (DEVNODE)(DeviceInfoData->DevInst),
                                               0) == CR_SUCCESS)
                        && !(status & DN_ROOT_ENUMERATED))
                    {
                        firmwarePort = TRUE;
                    }
                }
            }

            dwSize = sizeof(dwFirmwareIdentified);
            if (firmwarePort == FALSE &&
                RegQueryValueEx(hKey,
                                m_szFirmwareIdentified,
                                NULL,
                                NULL,
                                (PBYTE) &dwFirmwareIdentified,
                                &dwSize) == ERROR_SUCCESS) {

                //
                // ACPI puts the value "FirmwareIdentified" if it has enumerated 
                // this port.  We only rely on this if a DDN isn't present and we
                // couldn't get the enumerator name
                //
                firmwarePort = TRUE;
            }

            ZeroMemory(charBuffer, sizeof(charBuffer));
        }

        RegCloseKey(hKey);
    }

    if (firmwarePort) {
        //
        // Try to find "COM" in the name.  If it is found, simply extract 
        // the number that follows it and use that as the com number.
        //
        // Otherwise:
        // 1) try to determine the number of the com port based on its
        //    IO range, otherwise
        // 2) look through the com db and try to find an unused port from
        //    1 to 4, if none are present then let the DB pick the next open
        //    port number
        //
        if (comPort[0] != (TCHAR) 0) {
            _wcsupr(comPort);
            comLocation = wcsstr(comPort, m_szCOM);
            if (comLocation) {
                comPortNumber = myatoi(comLocation + wcslen(m_szCOM));
            }
        }
        
        if (comPortNumber == NO_COM_NUMBER && 
            !DetermineComNumberFromResources((DEVINST) DeviceInfoData->DevInst,
                                             &comPortNumber) &&
            (hComDB != HCOMDB_INVALID_HANDLE_VALUE) &&
            (ComDBGetCurrentPortUsage(hComDB,
                                      portUsage,
                                      MAX_COM_PORT / 8,
                                      CDB_REPORT_BITS,
                                      &portsReported) == ERROR_SUCCESS)) {
            if (!(portUsage[0] & 0x1)) {
                comPortNumber = 1;
            }
            else if (!(portUsage[0] & 0x2)) {
                comPortNumber = 2;
            }
            else if (!(portUsage[0] & 0x4)) {
                comPortNumber = 3;
            }
            else if (!(portUsage[0] & 0x8)) {
                comPortNumber = 4;
            }
            else {
                comPortNumber = NO_COM_NUMBER;
            }
        }
    }

    if (comPortNumber == NO_COM_NUMBER) {
        if (hComDB == HCOMDB_INVALID_HANDLE_VALUE) {
            //
            // Couldn't open the DB, pick a com port number that doesn't conflict
            // with any firmware ports
            //
            comPortNumber = DEF_MIN_COM_NUM;
        }
        else {
            //
            // Let the db find the next number
            //
            ComDBClaimNextFreePort(hComDB,
                                   &comPortNumber);
        }
    }
    else {
        //
        // We have been told what number to use, claim it irregardless of what
        // has already been claimed
        //
        ComDBClaimPort(hComDB,
                       comPortNumber,
                       TRUE,
                       NULL);
    }

    if (hComDB != HCOMDB_INVALID_HANDLE_VALUE) {
        ComDBClose(hComDB);
    }

    //
    // Generate the serial and COM port names based on the numbers we picked.
    //
    wsprintf(szPortName, TEXT("%s%d"), m_szCOM, comPortNumber);

    //
    // Write out Device Parameters\PortName and PollingPeriod
    //
    if((hKey = SetupDiCreateDevRegKey(DeviceInfoSet,
                                      DeviceInfoData,
                                      DICS_FLAG_GLOBAL,
                                      0,
                                      DIREG_DEV,
                                      NULL,
                                      NULL)) != INVALID_HANDLE_VALUE) {
        DWORD PollingPeriod = PollingPeriods[POLL_PERIOD_DEFAULT_IDX];

        //
        // A failure is not catastrophic, serial will just not know what to call 
        // the port
        //
        RegSetValueEx(hKey,
                      m_szPortName,
                      0,
                      REG_SZ,
                      (PBYTE) szPortName,
                      ByteCountOf(lstrlen(szPortName) + 1)
                      );

        RegSetValueEx(hKey,
                      m_szPollingPeriod,
                      0,
                      REG_DWORD,
                      (PBYTE) &PollingPeriod,
                      sizeof(DWORD)
                      );

        RegCloseKey(hKey);
    }

    //
    // Now do the installation for this device.
    //
    if(!SetupDiInstallDevice(DeviceInfoSet, DeviceInfoData)) {
        return GetLastError();
    }

    //
    // Write out the friendly name based on the device desc
    //
    if (LoadString(g_hInst, 
                   IDS_FRIENDLY_FORMAT, 
                   friendlyNameFormat, 
                   CharSizeOf(friendlyNameFormat)) &&
        SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                         DeviceInfoData,
                                         SPDRP_DEVICEDESC,
                                         NULL,
                                         (PBYTE)deviceDesc,
                                         sizeof(deviceDesc),
                                         NULL)) {
        wsprintf(charBuffer, friendlyNameFormat, deviceDesc, szPortName);
    }
    else {
        lstrcpy(charBuffer, szPortName);
    }

    // Write the string friendly name string out
    SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                     DeviceInfoData,
                                     SPDRP_FRIENDLYNAME,
                                     (PBYTE)charBuffer,
                                     ByteCountOf(lstrlen(charBuffer) + 1)
                                     );

    // 
    // Write out the default settings to win.ini (really a registry key) if they
    // don't already exist.
    //
    wcscat(szPortName, m_szColon);
    charBuffer[0] = TEXT('\0');
    GetProfileString(m_szPorts,
                     szPortName,
                     TEXT(""),
                     charBuffer,
                     sizeof(charBuffer) / sizeof(TCHAR) );
    //
    // Check to see if the default string provided was copied in, if so, write
    // out the port defaults
    //
    if (charBuffer[0] == TEXT('\0')) {
        WriteProfileString(m_szPorts, szPortName, m_szDefParams);
    }

    return NO_ERROR;
}
    
// @@BEGIN_DDKSPLIT
BOOL
GetSerialPortDevInstConfig(
    IN  DEVINST            DevInst,
    IN  ULONG              LogConfigType,
    OUT PIO_RESOURCE       IoResource,             OPTIONAL
    OUT PIRQ_RESOURCE      IrqResource             OPTIONAL
    )
/*++

Routine Description:

    This routine retrieves the base IO port and IRQ for the specified device instance
    in a particular logconfig.

Arguments:

    DevInst - Supplies the handle of a device instance to retrieve configuration for.

    LogConfigType - Specifies the type of logconfig to retrieve.  Must be either
        ALLOC_LOG_CONF, BOOT_LOG_CONF, or FORCED_LOG_CONF.

    IoResource - Optionally, supplies the address of an Io resource structure 
        that receives the Io resource retreived.

    IrqResource - Optionally, supplies the address of an IRQ resource variable 
        that receives the IRQ resource retrieved.

    AdditionalResources - Optionally, supplies the address of a CM_RESOURCE_LIST pointer.
        If this parameter is specified, then this pointer will be filled in with the
        address of a newly-allocated buffer containing any additional resources contained
        in this logconfig.  If there are no additional resources (which will typically be
        the case), then this pointer will be set to NULL.

        The caller is responsible for freeing this buffer.

    AdditionalResourcesSize - Optionally, supplies the address of a variable that receives
        the size, in bytes, of the buffer allocated and returned in the AdditionalResources
        parameter.  If that parameter is not specified, then this parameter is ignored.

Return Value:

    If success, the return value is TRUE, otherwise it is FALSE.

--*/
{
    LOG_CONF LogConfig;
    RES_DES ResDes;
    CONFIGRET cr;
    BOOL Success;
    PBYTE ResDesBuffer = NULL;
    ULONG ResDesBufferSize = 68; // big enough for everything but class-specific resource.

    if(CM_Get_First_Log_Conf(&LogConfig, DevInst, LogConfigType) != CR_SUCCESS) {
        return FALSE;
    }

    Success = FALSE;    // assume failure.

    //
    // First, get the Io base port
    //
    if(IoResource) {

        if(CM_Get_Next_Res_Des(&ResDes, LogConfig, ResType_IO, NULL, 0) != CR_SUCCESS) {
            goto clean0;
        }

        cr = CM_Get_Res_Des_Data(ResDes, IoResource, sizeof(IO_RESOURCE), 0);

        CM_Free_Res_Des_Handle(ResDes);

        if(cr != CR_SUCCESS) {
            goto clean0;
        }
    }

    //
    // Now, get the IRQ
    //
    if(IrqResource) {

        if(CM_Get_Next_Res_Des(&ResDes, LogConfig, ResType_IRQ, NULL, 0) != CR_SUCCESS) {
            goto clean0;
        }

        cr = CM_Get_Res_Des_Data(ResDes, IrqResource, sizeof(IRQ_RESOURCE), 0);

        CM_Free_Res_Des_Handle(ResDes);

        if(cr != CR_SUCCESS) {
            goto clean0;
        }
    }

    Success = TRUE;

clean0:
    CM_Free_Log_Conf_Handle(LogConfig);

    if(ResDesBuffer) {
        LocalFree(ResDesBuffer);
    }

    return Success;
}
// @@END_DDKSPLIT

void InitStrings(void)
{
    DWORD  dwClass, dwShare;
    TCHAR  szClass[ 40 ];

    LoadString(g_hInst, 
               INITS,
               g_szErrMem,
               CharSizeOf(g_szErrMem));
    LoadString(g_hInst, 
               IDS_INIT_NAME,
               g_szPortsApplet,
               CharSizeOf(g_szPortsApplet));

    //
    //  Get the "Close" string
    //
    LoadString(g_hInst,
               IDS_INIT_CLOSE,
               g_szClose, 
               CharSizeOf(g_szClose));
}
