#define STANDALONE
#include <apitest.h>

extern void func_AssocQueryString(void);
extern void func_CharUpperNoDBCS(void);
extern void func_PathFileExistsDefExtAndAttributesW(void);
extern void func_PathFindOnPath(void);
extern void func_IShellFolderHelpers(void);
extern void func_IsQSForward(void);
extern void func_IStreamPidl(void);
extern void func_NextPath(void);
extern void func_PathIsUNC(void);
extern void func_PathIsUNCServer(void);
extern void func_PathIsUNCServerShare(void);
extern void func_PathUnExpandEnvStrings(void);
extern void func_PathUnExpandEnvStringsForUser(void);
extern void func_QuerySourceCreateFromKey(void);
extern void func_SHAreIconsEqual(void);
extern void func_SHGetRestriction(void);
extern void func_SHInvokeCommandsOnContextMenu(void);
extern void func_SHLoadIndirectString(void);
extern void func_SHLoadRegUIString(void);
extern void func_SHPropertyBag(void);
extern void func_StrDup(void);
extern void func_StrFormatByteSizeW(void);
extern void func_StrToInt(void);

const struct test winetest_testlist[] =
{
    { "AssocQueryString", func_AssocQueryString },
    { "CharUpperNoDBCS", func_CharUpperNoDBCS },
    { "PathFileExistsDefExtAndAttributesW", func_PathFileExistsDefExtAndAttributesW },
    { "PathFindOnPath", func_PathFindOnPath },
    { "IShellFolderHelpers", func_IShellFolderHelpers },
    { "IsQSForward", func_IsQSForward },
    { "IStreamPidl", func_IStreamPidl },
    { "NextPath", func_NextPath },
    { "PathIsUNC", func_PathIsUNC },
    { "PathIsUNCServer", func_PathIsUNCServer },
    { "PathIsUNCServerShare", func_PathIsUNCServerShare },
    { "PathUnExpandEnvStrings", func_PathUnExpandEnvStrings },
    { "PathUnExpandEnvStringsForUser", func_PathUnExpandEnvStringsForUser },
    { "QuerySourceCreateFromKey", func_QuerySourceCreateFromKey },
    { "SHAreIconsEqual", func_SHAreIconsEqual },
    { "SHGetRestriction", func_SHGetRestriction },
    { "SHInvokeCommandsOnContextMenu", func_SHInvokeCommandsOnContextMenu },
    { "SHLoadIndirectString", func_SHLoadIndirectString },
    { "SHLoadRegUIString", func_SHLoadRegUIString },
    { "SHPropertyBag", func_SHPropertyBag },
    { "StrDup", func_StrDup },
    { "StrFormatByteSizeW", func_StrFormatByteSizeW },
    { "StrToInt", func_StrToInt },
    { 0, 0 }
};
