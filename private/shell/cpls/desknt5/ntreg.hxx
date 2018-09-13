/**************************************************************************\
* Module Name: ntreg.hxx
*
* CRegistrySettings class
*
*  This class handles getting registry information for display driver
*  information.
*
* Copyright (c) Microsoft Corp.  1992-1998 All Rights Reserved
*
\**************************************************************************/


class CRegistrySettings
{
protected:

    //
    // The Display Device we are currently working with.
    //

    HKEY   hkVideoRegR;
    HKEY   hkVideoRegW;
    HKEY   hkServiceReg;
    LPTSTR pszDrvName;
    LPTSTR pszKeyName;
    LPTSTR pszDevInstanceId;
    BOOL   bdisablable;

public:

    CRegistrySettings(LPTSTR pstrDeviceKey);
    ~CRegistrySettings();

    LPTSTR CloneDescription(void);
    LPTSTR CloneDisplayFileNames(BOOL bPreprocess);

    //
    // Returns a pointer to the mini port name.  THIS IS NOT A CLONE!
    // THE CALLER MUST COPY IT IF IT NEEDS TO KEEP IT AROUND!
    //
    // DO NOT FREE THE POINTER RETURNED FROM THIS CALL!
    //

    LPTSTR GetMiniPort(void)         { return pszDrvName; }
    LPTSTR GetKeyName(void)          { return pszKeyName; }

    VOID GetHardwareInformation(PDISPLAY_REGISTRY_HARDWARE_INFO pInfo);
    void GetDeviceInstanceId(LPWSTR lpwDeviceId, int cch);


};

//Maximum length of a device instance ID.

#define DEV_INSTANCE_ID_LENGTH  128


