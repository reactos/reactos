#define __ROS_LONG64__

#define STANDALONE
#include <wine/test.h>

extern void func_CMyComputer(void);
extern void func_CShellDesktop(void);
extern void func_CShellLink(void);
extern void func_menu(void);
extern void func_SHParseDisplayName(void);
extern void func_shlexec(void);

const struct test winetest_testlist[] =
{
    { "CMyComputer", func_CMyComputer },
    { "CShellDesktop", func_CShellDesktop },
    { "CShellLink", func_CShellLink },
    { "menu", func_menu },
    { "SHParseDisplayName", func_SHParseDisplayName },
    { "shlexec", func_shlexec },
    { 0, 0 }
};
