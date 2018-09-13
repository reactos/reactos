/************************************************************************\
*
* MODULE: exts.h
*
* DESCRIPTION: macro driving file for use with stdexts.h and stdexts.c.
*
* Copyright (c) 6/9/1995, Microsoft Corporation
*
* 6/9/1995 SanfordS Created
* 10/28/97 butchered by cdturner to work for the shell team
*
\************************************************************************/

DOIT(   help
        ,"help -v [cmd]                  - Displays this list or gives details on command\n"
        ,"  help      - To dump short help text on all commands.\n"
         "  help -v   - To dump long help text on all commands.\n"
         "  help cmd  - To dump long help on given command.\n"
        ,"v"
        ,CUSTOM)

DOIT(   pidl
        ,"pidl -vrf [address]            - Display contents of pidl at address [address]\n"
        ,"pidl -r [address]   - To dump the contents of a RegItem pidl at [address]\n"
         "pidl -f [address]   - To dump the contents of a FileSys pidl at [address]\n"
         "pidl -vr [address]  - Dumps the verbose info for each pidl\n"
        , "vrf" 
        , STDARGS1)

DOIT(   dso
        ,"dso -l <Struct> [Field] [Addr] - Dumps Struct field(s)'s offset(s) & value(s)\n"
        ,"dso -l                         - List the structures known [long]\n"
         "dso SHELLSTATEA [Addr]         - Dump the SHELLSTATEA struct at [Addr]\n"
         "dso SHELLSTATEA version [Addr] - Dump the version field of the SHELLSTATEA\n"
         "                                 struct at [addr]\n"
        ,"l"
        ,CUSTOM)

DOIT(   flags
        ,"flags -l <flagsname> val|*addr - Displays the flags for the bitfields\n"
        ,"flags <flagsname> val          - list the flags of type <flagname> in val\n"
         "flags <flagsname> *addr        - list the flags of type <flagname> at addr\n"
         "flags -l                       - list the known options for <flagsname>\n"
        ,"l"
        ,CUSTOM)

DOIT(   filever
        ,"filever -n [<filename>]        - Display the shell ver info, or filename ver.\n"
        ,"filever                        - Both DllGetVersionInfo and file version of Shell32\n"
         "filever -n                     - Only display file version info (no LoadLibrary)\n"
         "filever <filename>             - Display the file version of <filename>\n"
        ,"n"
        ,CUSTOM)

DOIT(   test
        ,"test                           - Test basic debug functions.\n"
        ,""
        ,""
        ,NOARGS)

DOIT(   ver
        ,"ver                            - show versions of shlexts.\n"
        ,""
        ,""
        ,NOARGS)

