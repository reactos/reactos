/***
 **
 **     Module: reg.
 **
 **     Description:
 **       This is the public interface for getting the registry key
 **       location for various Office 95 fetaures.
 **
 **       Currently, the settings are all located under the same root,
 **       though possibly under wither HKEY_CURRENT_USER or
 **       HKEY_LOCAL_MACHINE. This location is:
 **       <HKEY>\Software\Microsoft\Microsoft Office\95\".
 **
 **     Author: Michael Jansson
 **     Created: 12\19\94
 **
 ***/

extern const TCHAR vcszKeyAnthem[];
extern const TCHAR vcszKeyFileNewNFT[];
extern const TCHAR vcszKeyFileNewLocal[];
extern const TCHAR vcszKeyFileNewShared[];
extern const TCHAR vcszKeyIS[];
extern const TCHAR vcszSubKeyISToWHelp[];
extern const TCHAR vcszKeyAutoCorrectSettings[];
extern const TCHAR vcszKeyAutoCorrectRepl[];
extern const TCHAR vcszKeyAutoCorrectDefaultRepl[];
extern const TCHAR vcszSubKeyAutoInitial[];
extern const TCHAR vcszSubKeyAutoCapital[];
extern const TCHAR vcszSubKeyReplace[];
extern const TCHAR vcszNoTracking[];
#ifdef WAIT3340
extern const TCHAR vcszMSHelp[];
#endif
