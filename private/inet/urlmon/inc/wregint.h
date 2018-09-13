#ifndef _REG_API_DEFINED_
#define _REG_API_DEFINED_

#ifdef unused
#define RegConnectRegistry  RegConnectRegistryW
#define RegConnectRegistry  RegConnectRegistryA
#define RegCreateKey  RegCreateKeyW
#define RegCreateKey  RegCreateKeyA
#define RegCreateKeyEx  RegCreateKeyExW
#define RegCreateKeyEx  RegCreateKeyExA
#define RegDeleteKey  RegDeleteKeyW
#define RegDeleteKey  RegDeleteKeyA
#define RegDeleteValue  RegDeleteValueW
#define RegDeleteValue  RegDeleteValueA
#define RegEnumKey  RegEnumKeyW
#define RegEnumKey  RegEnumKeyA
#define RegEnumKeyEx  RegEnumKeyExW
#define RegEnumKeyEx  RegEnumKeyExA
#define RegEnumValue  RegEnumValueW
#define RegEnumValue  RegEnumValueA
#define RegLoadKey  RegLoadKeyW
#define RegLoadKey  RegLoadKeyA
#define RegOpenKey  RegOpenKeyW
#define RegOpenKey  RegOpenKeyA
#define RegOpenKeyEx  RegOpenKeyExW
#define RegOpenKeyEx  RegOpenKeyExA
#define RegQueryInfoKey  RegQueryInfoKeyW
#define RegQueryInfoKey  RegQueryInfoKeyA
#define RegQueryValue  RegQueryValueW
#define RegQueryValue  RegQueryValueA
#define RegQueryMultipleValues  RegQueryMultipleValuesW
#define RegQueryMultipleValues  RegQueryMultipleValuesA
#define RegQueryValueEx  RegQueryValueExW
#define RegQueryValueEx  RegQueryValueExA
#define RegReplaceKey  RegReplaceKeyW
#define RegReplaceKey  RegReplaceKeyA
#define RegRestoreKey  RegRestoreKeyW
#define RegRestoreKey  RegRestoreKeyA
#define RegSaveKey  RegSaveKeyW
#define RegSaveKey  RegSaveKeyA
#define RegSetValue  RegSetValueW
#define RegSetValue  RegSetValueA
#define RegSetValueEx  RegSetValueExW
#define RegSetValueEx  RegSetValueExA
#define RegUnLoadKey  RegUnLoadKeyW
#define RegUnLoadKey  RegUnLoadKeyA
#define InitiateSystemShutdown  InitiateSystemShutdownW
#define InitiateSystemShutdown  InitiateSystemShutdownA
#define AbortSystemShutdown  AbortSystemShutdownW
#define AbortSystemShutdown  AbortSystemShutdownA


#undef RegConnectRegistry
#undef RegCreateKey
#undef RegCreateKeyEx
#undef RegDeleteKey
#undef RegDeleteValue
#undef RegEnumKey
#undef RegEnumKeyEx
#undef RegEnumValue
#undef RegLoadKey
#undef RegOpenKey
#undef RegOpenKeyEx
#undef RegQueryInfoKey
#undef RegQueryValue
#undef RegQueryMultipleValues
#undef RegQueryValueEx
#undef RegReplaceKey
#undef RegRestoreKey
#undef RegSaveKey
#undef RegSetValue
#undef RegSetValueEx
#undef RegUnLoadKey
#undef InitiateSystemShutdown
#undef AbortSystemShutdown

#endif //unused

#pragma warning(disable:4005)               // re-enable below


#include "..\utils\wreg.hxx"
extern CRegistryA *g_vpReg;


#define RegConnectRegistry            (g_vpReg)->ConnectRegistry
#define RegCloseKey                   (g_vpReg)->CloseKey
#define RegCreateKey                  (g_vpReg)->CreateKey
#define RegCreateKeyEx                (g_vpReg)->CreateKeyEx
#define RegDeleteKey                  (g_vpReg)->DeleteKey
#define RegDeleteValue                (g_vpReg)->DeleteValue
#define RegEnumKey                    (g_vpReg)->EnumKey
#define RegEnumKeyEx                  (g_vpReg)->EnumKeyEx
#define RegEnumValue                  (g_vpReg)->EnumValue
#define RegFlushKey                   (g_vpReg)->FlushKey
#define RegLoadKey                    (g_vpReg)->LoadKey
#define RegOpenKey                    (g_vpReg)->OpenKey
#define RegOpenKeyEx                  (g_vpReg)->OpenKeyEx
#define RegQueryInfoKey               (g_vpReg)->QueryInfoKey
#define RegQueryValue                 (g_vpReg)->QueryValue
#define RegQueryMultipleValues        (g_vpReg)->QueryMultipleValues
#define RegQueryValueEx               (g_vpReg)->QueryValueEx
#define RegReplaceKey                 (g_vpReg)->ReplaceKey
#define RegRestoreKey                 (g_vpReg)->RestoreKey
#define RegSaveKey                    (g_vpReg)->SaveKey
#define RegSetValue                   (g_vpReg)->SetValue
#define RegSetValueEx                 (g_vpReg)->SetValueEx
#define RegUnLoadKey                  (g_vpReg)->UnLoadKey

#pragma warning(default:4005)               // re-enable below

#endif //_REG_API_DEFINED_
