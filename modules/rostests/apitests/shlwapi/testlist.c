#define STANDALONE
#include <apitest.h>

extern void func_AssocQueryString(void);
extern void func_PathFileExistsDefExtAndAttributesW(void);
extern void func_PathFindOnPath(void);
extern void func_IShellFolderHelpers(void);
extern void func_isuncpath(void);
extern void func_isuncpathserver(void);
extern void func_isuncpathservershare(void);
extern void func_PathUnExpandEnvStrings(void);
extern void func_PathUnExpandEnvStringsForUser(void);
extern void func_SHAreIconsEqual(void);
extern void func_SHGetRestriction(void);
extern void func_SHLoadIndirectString(void);
extern void func_SHLoadRegUIString(void);
extern void func_SHPropertyBag(void);
extern void func_StrDup(void);
extern void func_StrFormatByteSizeW(void);

const struct test winetest_testlist[] =
{
    { "AssocQueryString", func_AssocQueryString },
    { "PathFileExistsDefExtAndAttributesW", func_PathFileExistsDefExtAndAttributesW },
    { "PathFindOnPath", func_PathFindOnPath },
    { "IShellFolderHelpers", func_IShellFolderHelpers },
    { "PathIsUNC", func_isuncpath },
    { "PathIsUNCServer", func_isuncpathserver },
    { "PathIsUNCServerShare", func_isuncpathservershare },
    { "PathUnExpandEnvStrings", func_PathUnExpandEnvStrings },
    { "PathUnExpandEnvStringsForUser", func_PathUnExpandEnvStringsForUser },
    { "SHAreIconsEqual", func_SHAreIconsEqual },
    { "SHGetRestriction", func_SHGetRestriction },
    { "SHLoadIndirectString", func_SHLoadIndirectString },
    { "SHLoadRegUIString", func_SHLoadRegUIString },
    { "SHPropertyBag", func_SHPropertyBag },
    { "StrDup", func_StrDup },
    { "StrFormatByteSizeW", func_StrFormatByteSizeW },
    { 0, 0 }
};
