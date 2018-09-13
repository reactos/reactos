#ifndef __SETUPENUM_H_
#define __SETUPENUM_H_

class COCSetupApp
{
public:

    COCSetupApp();
    ~COCSetupApp();
    
    BOOL GetAppInfo(APPINFODATA *pai);
    BOOL ReadFromKey(HKEY hkey);
    BOOL Run();

    TCHAR _szDisplayName[MAX_PATH];

protected:
    TCHAR _szApp[MAX_PATH];
    TCHAR _szArgs[MAX_PATH];
};

class COCSetupEnum
{
public:
    COCSetupEnum();
    ~COCSetupEnum();

    BOOL EnumOCSetupItems();
    BOOL Next(COCSetupApp **);

    static BOOL s_OCSetupNeeded();

protected:
    HKEY _hkeyRoot;
    int _iRegEnumIndex;     // used to walk through the items
};

#endif //__SETUPENUM_H_
