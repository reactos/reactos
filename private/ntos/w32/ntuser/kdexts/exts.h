/************************************************************************\
*
* MODULE: exts.h
*
* DESCRIPTION: macro driving file for use with stdexts.h and stdexts.c.
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* 6/9/1995 SanfordS Created
*
\************************************************************************/

DOIT(   help
        ,"help -v [cmd]                 - Displays this list or gives details on command\n"
        ,"  help      - To dump short help text on all commands.\n"
         "  help -v   - To dump long help text on all commands.\n"
         "  help cmd  - To dump long help on given command.\n"
        ,"v"
        ,CUSTOM)

#ifdef KERNEL
DOIT(   atom
        ,"atom                          - Dump atoms or atom tables\n"
        ,"atom -gl [atom]\n"
        ,"gl"
        ,STDARGS1)
#endif // KERNEL

#ifndef KERNEL
DOIT(   cbp
        ,"cbp                           - Breaks into the Debugger on CSRSS.EXE\n"
        ,""
        ,""
        ,NOARGS)
#endif

#ifndef KERNEL
DOIT(   dci
        ,"dci                           - Dump process client info.\n"
        ,""
        ,""
        ,NOARGS)
#endif //!KERNEL

#ifdef KERNEL
DOIT(   dcls
        ,"dcls [pcls]                   - Dump window class\n"
        ,"  dcls -v     - for verbose info.\n"
         "  dcls        - to list all classes.\n"
         "  dcls -p ppi - to list all classes for process ppi\n"
        ,"pv"
        ,STDARGS1)
#endif // KERNEL

#ifdef KERNEL
DOIT(   dcur
        ,"dcur -aivp [pcur]             - Dump cursors\n"
        ,"  dcur         - dump all cursor objects.\n"
         "  dcur -i <id> - dump cursors with given res id.\n"
         "  dcur -p <ppi>- dump cursors owned by ppi.\n"
         "  dcur -v      - dump in verbose format.\n"
         "  dcur -a      - expand dump of ACONs.\n"
         "  dcur <pcur>  - dump given cursor handle or pointer.\n"
        ,"aivp"
        ,STDARGS1)
#endif // KERNEL

#ifdef KERNEL
DOIT(   dde
        ,"dde -vr [conv|window|xact]    - Dump DDE tracking information\n"
        ,"  v - verbose\n"
         "  r - recurse to inner structures 1 level\n"
         "  window object - dumps all convs associated w/window\n"
         "  conv object - dumps conversation.\n"
         "  xact object - dumps transaction.\n"
        ,"vr"
        ,STDARGS1)
#endif // KERNEL

#ifdef KERNEL
DOIT(   ddl
        ,"ddl [pdesk]                   - Dump desktop log\n"
        ,"  ddl pdesk    - dumps the desktop log for pdesk.\n"
         "  ddl -v pdesk - dumps the desktop log for pdesk with stack traces.\n"
        ,"v"
        ,STDARGS1)
#endif // KERNEL

#ifndef KERNEL
DOIT(   ddeml
        ,"ddeml -v [i<inst>] [t<type>] [hObj|pObj] - Dump DDEML state information\n"
        ,"  ddeml                     - lists all ddeml instances for this process\n"
         "  ddeml t<type>             - lists all ddeml objects of the given type\n"
         "  type 0 = All types\n"
         "  type 1 = Instances\n"
         "  type 2 = Server Conversations\n"
         "  type 3 = Client Conversations\n"
         "  type 4 = Conversation Lists\n"
         "  type 5 = Transactions\n"
         "  type 6 = Data Handles\n"
         "  type 7 = Zombie Conversations\n"
         "  ddeml i<instance> t<type> - restricts listing to one instance.\n"
         "  ddeml hObj                - dumps ddeml object\n"
         "  adding a 'v' simply turns lists into dumps.\n"
        ,"v"
        ,CUSTOM)
#endif //!KERNEL

#ifdef KERNEL
DOIT(   ddesk
        ,"ddesk -vh <pdesk>             - Displays objects allocated in desktop\n"
        ,"  ddesk           - dumps list of desktops\n"
         "  ddesk address   - dumps simple statistics for desktop\n"
         "  ddesk v address - dumps verbose statistics for desktop\n"
         "  ddesk h address - dumps statistics for desktop plus handle list\n"
        ,"vh"
        ,STDARGS1)
#endif // KERNEL

#ifdef KERNEL
DOIT(   ddk
        ,"ddk <pKbdTbl>                 - Dump deadkey table\n"
        ,"  ddk pKbdTbl   - Dump the deadkey table at pKbdTbl\n"
        ,""
        ,STDARGS1)
#endif

DOIT(   df
        ,"df [flags] | [-p pid]         - Displays or sets debug flags\n"
        ,"  df            - display debug flags\n"
         "  df [flags]  - enter new flags in format <Detail><Print><Prompt> \n"
         "      <Detail>    = [0-3] Print File/Line = 1, Hide PID/Component = 2\n"
         "      <Print>     = [0-7] Errors = 1, Warnings = 2, Verbose = 4\n"
         "      <Prompt>    = [0-7] Errors = 1, Warnings = 2, Verbose = 4\n"
         "    The default is 031\n"
         "  df -p pid   - shows rips only for this pid or 0 for all\n"
        ,"px"
        ,CUSTOM)

DOIT(   dhe
        ,"dhe [pointer|handle] | [-t[o[p]] type [pti/ppi]] - Dump handle entry(ies)\n"
        ,"  dhe            - dumps all handle entrys\n"
         "      -t dumps of all handles of the given type\n"
         "      -o all handles owned by pti/ppi\n"
         "      -op all handles owned by ppi or any thread in this process\n"
        ,"orpt"
        ,STDARGS2)

#ifdef KERNEL
DOIT(   dhk
        ,"dhk -ag [pti]                 - Dump hooks\n"
        ,"  dhk            - dumps local hooks on the foreground thread\n"
         "  dhk -g         - dumps global hooks\n"
         "  dhk pti        - dumps local hooks on THREADINFO at pti\n"
         "  dhk -g pti     - dumps global hooks and local hooks on THREADINFO at pti\n"
         "  dhk -a         - dumps local hooks for all threads\n"
         "  dhk -ag        - dumps global hooks and local hooks for all threads\n"
         ,"ga"
         ,STDARGS1)
#endif

#ifdef KERNEL
DOIT(   dhot
        ,"dhot                          - Dump registered hotkeys\n"
        ,""
        ,""
        ,NOARGS)
#endif

#ifdef KERNEL
DOIT(   dhs
        ,"dhs -vpty [id|type]           - Dump handle table statistics\n"
        ,"  dhs           - dumps simple statistics for whole table\n"
         "  dhs -t id     - dumps simple statistics for objects created by thread id\n"
         "  dhs -p id     - dumps simple statistics for objects created by process id\n"
         "  dhs -v        - dumps verbose statistics\n"
         "  dhs -y type   - dumps statistics for objects of type.\n"
        ,"tpvy"
        ,STDARGS1)
#endif

#ifdef KERNEL
DOIT(   di
        ,"di                            - Displays USER input processing globals.\n"
        ,""
        ,""
        ,NOARGS)
#endif

#if defined(KERNEL)
DOIT(   dii
        ,"dii <piiex>                   - Dump extended IME information\n"
        ,"  dii piiex     - Dump the extended IME information at piiex\n"
         "  dii -h hkl    - Dump the extended IME information of hkl\n"
        ,"h"
        ,STDARGS1)
#endif


DOIT(   dimc
        ,"dimc [-hrvus] -[wci] [imc|wnd,etc.] - Dump Input Context\n"
        ,"  dimc [opts] <himc|pimc>      - dumps kernel side imc\n"
         "  dimc [opts] -w wnd           - dumps kernel side imc of wnd\n"
         "  dimc [opts] -c <pClientImc>     - dumps client side imc\n"
         "  dimc [opts] -i <pInputContext>  - dumps inputcontext\n"
         "       -h                      - show open, conversion, sentence mode and handles\n"
         "       -d                      - show hCompStr, hCanInfo etc (includes -h).\n"
         "       -r                      - recurse pImeModeSaver\n"
         "       -v                      - verbose\n"
         "       -u                      - assume UNICODE (use with -i)\n"
         "       -s                      - show relatively raw Composition String\n"
         "       -a                      - all\n"
        ,"wcihdrvusa"
        ,STDARGS1)

#ifdef KERNEL
DOIT(   dimk
        ,"dimk [pImeHotKeyObj]          - Dump IME Hotkeys\n"
        ,""
        ,""
        ,STDARGS1)
#endif

#if !defined(KERNEL)
DOIT(   dimedpi
        ,""
        ,"  dimedpi          - Dump all IMEDPI\n"
         "  dimedpi <addr>   - Dump IMEDPI at <addr>\n"
         "  dimedpi -v       - Dump with !dso format\n"
         "  dimedpi -i       - Dump ImeInfo\n"
        ,"vi"
        ,STDARGS1)
#endif

#ifdef KERNEL
DOIT(   dinp
        ,"dinp -v [pDeviceInfo]         - Dump input diagnostics\n"
        ,"  dinp <addr>    - Complete DeviceInfo struct at <addr>\n"
         "  dinp           - Short summary of all input devices\n"
         "  dinp -i <addr> - Dump input records for DeviceInfo struct at <addr>\n"
         "  dinp -v        - Dump complete DeviceInfo for all devices\n"
        ,"iv"
        ,STDARGS1)

DOIT(   dkl
        ,"dkl -akv <pkl>                - Dump keyboard layout structures\n"
        ,"  dkl pkl       - Dump the keyboard layout structure at pkl\n"
         "  dkl -a pkl    - Dump all keyboard layout structures beginning with pkl\n"
         "  dkl -k[v]     - Dump pklActive for all threads (v for verbose)\n"
        ,"akv"
        ,STDARGS1)
#endif

DOIT(   disi
        ,""
        ,"disi <pinput>                 - Dump event injection union\n"
         ""
        ,""
        ,STDARGS1)

DOIT(   ddlgt
        ,""
        ,"ddlgt <pdlgTemplate>          - Dump dialog template\n"
         "  ddlgt -v <pdlgTemplate>     - verbose\n"
         "  ddlgt -r <pdlgTemplate>     - print dialog items\n"
        ,"vr"
        ,STDARGS1)

DOIT(   dll
        ,"dll [*]addr [l#] [b#] [o#] [c#] [t[addr]] - Dump linked list (can Ctrl-C)\n"
        ,"  dll addr    - dumps list starting at addr 8 DWORDs each structure,\n"
         "                assumes link is first DWORD, w/NULL termination.\n"
         "  dll *addr   - same except starts at *addr\n"
         "  dll         - dumps next group of structures w/same options\n"
         "  \nOptions:\n"
         "  dll addr l3 - dumps 3 DWORDs per structure (default 8)\n"
         "  dll addr b2 - starts dump 2 DWORDs back - for LIST_ENTRYs (default 0)\n"
         "  dll addr o4 - next link is 4 DWORDs from top of structure (default 0)\n"
         "  dll addr c5 - dumps 5 structures only (defaults to 25)\n"
         "  dll addr l3 b2 o4 c5 - combines the option of the four previous lines\n"
         "  dll addr t  - counts (not dumps) items in list, tests for loop\n"
         "  dll addr t<addr>  - as above, halts if item at <addr> is found\n"
         "  dll addr t c5000  - as above, but limit c5000 (t default is c100000)\n"
        ,""
        ,CUSTOM)

#ifdef KERNEL
DOIT(   dlr
        ,"dlr <pointer|handle>          - Displays assignment locks for object\n"
        ,""
        ,""
        ,STDARGS1)
#endif //KERNEL

DOIT(   dm
        ,"dm -vris <menu|window>        - Dumps a menu\n"
        ,"  -v   - Verbose information mode\n"
         "  -r   - Recurse through sub-menus\n"
         "  -i   - Ignore items in the menu\n"
         "  -s   - dump a window's System menu\n"
        ,"virs"
        ,STDARGS1)

DOIT(   dmon
        ,"dmon <pMonitor>               - Dump MONITOR\n"
        ,"  dmon <pMonitor> - dump MONITOR at address\n"
        ,""
        ,STDARGS1)

#ifdef KERNEL
DOIT(   dmq
        ,"dmq [-ac] [pq]                - Messages in queues\n"
        ,"  dmq [pq]  - lists messages in queue specified (default gpqForeground)\n"
         "  dmq -a    - lists messages in all queues\n"
         "  dmq -c    - counts messages in all queues\n"
        ,"ac"
        ,STDARGS1)
#endif

#ifdef KERNEL
DOIT(   dms
        ,"dms <MenuState>               - Dumps a pMenuState\n"
        ,""
        ,""
        ,STDARGS1)
#endif // KERNEL

#ifdef KERNEL
DOIT(   dp
        ,"dp -vcpt  [id]                - Displays simple process information\n"
        ,"  dp            - dumps simple process info of all W32 processes\n"
         "  dp -v         - verbose output\n"
         "  dp id/pep     - dumps process with given id or PEPROCESS pointer\n"
         "  dp -p ppi     - takes a ppi instead of a process id/pep\n"
         "  dp -c         - dumps current process for the current processor.\n"
         "  dp -t         - dumps threads for given process.\n"
        ,"vpct"
        ,STDARGS1)
#endif

#ifndef KERNEL
DOIT(   dped
        ,"dped <ped>                    - Dump PEDitControl structure\n"
        ,""
        ,""
        ,STDARGS1)
#endif //!KERNEL

#ifdef KERNEL
#if 0
DOIT(   dphys
        ,"dphys [pdi]                   - Dump PHYSICAL_DEV_INFO\n"
        ,"  dphys          - dump gpPhysDevInfo\n"
         "  dphys address  - dump PHYSICAL_DEV_INFO at address\n"
        ,""
        ,STDARGS1)
#endif
#endif

#ifdef KERNEL
DOIT(   dpi
        ,"dpi [ppi]                     - Displays PROCESSINFO structure specified\n"
        ,"  dpi -c         - dumps PROCESSINFO for current process\n"
         "  dpi address    - dumps PROCESSINFO structure at address\n"
         "  dpi            - dumps all PROCESSINFO structures\n"
        ,"c"
        ,STDARGS1)
#endif // KERNEL

#ifdef KERNEL
DOIT(   dpm
        ,"dpm <ppopupmenu>              - Dumps a popupmenu\n"
        ,""
        ,""
        ,STDARGS1)
#endif // KERNEL

#ifdef KERNEL
DOIT(   dq
        ,"dq -t [pq]                    - Displays Q structure specified\n"
        ,"  dq address    - dumps queue structure at address\n"
         "  dq -t address - dumps queue structure at address plus THREADINFO\n"
         "  dq -a         - dumps all queues\n"
         "  dq            - dumps gpqForeground\n"
        ,"at"
        ,STDARGS1)
#endif // KERNEL

DOIT(   dsbt
        ,"dsbt <pSBTrack>               - Displays Scroll Bar Track structure\n"
        ,"  dsbt pti       - dumps SBTrack info in pti\n"
        ,""
        ,STDARGS1)

DOIT(   dsbwnd
        ,"dsbwnd <psbwnd>               - Dump extra fields of Scrollbar windows\n"
        ,""
        ,""
        ,STDARGS1)

DOIT(   dsi
        ,"dsi [-bchmopvw]               - Displays SERVERINFO struct\n"
        ,"dsi -b    - adds mpFnid_serverCBWndProc info.\n"
         "dsi -c    - adds aszSysColor info.\n"
         "dsi -h    - adds SHAREDINFO info.\n"
         "dsi -m    - adds system metrics data.\n"
         "dsi -o    - adds oemInfo info.\n"
         "dsi -p    - adds mpFnidPfn info.\n"
         "dsi -v    - verbose shows rarely needed fields.\n"
         "dsi -w    - adds aStoCidPfn info.\n"
        ,"bchjmopvw"
        ,STDARGS0)

#ifdef KERNEL
DOIT(   dsms
        ,"dsms -vl [psms]               - Displays SMS (SendMessage structure) specified\n"
        ,"  dsms           - dumps all send message structures\n"
         "  dsms -c        - counts messages, doesn't display any\n"
         "  dsms -v        - dumps all verbose\n"
         "  dsms -m msg    - dumps all sms with message == msg\n"
         "  dsms address   - dumps specific sms\n"
         "  dsms -v address- dumps verbose\n"
         "  dsms -l address- dumps SendList of sms\n"
        ,"clmv"
        ,STDARGS1)
#endif // KERNEL

DOIT(   dso
        ,"dso <Struct> [Field] [addr [*n]]- Dumps Struct field(s)'s offset(s) & value(s)\n"
        ,"dso -v   - Dumps also the nested structures.\n"
         "    *n   - Dumps n structs from an array at starting at addr (default = 1)\n"
        ,"v"
        ,CUSTOM)

#ifdef KERNEL
DOIT(   dt
        ,"dt -gvcp [id]                 - Displays simple thread information\n"
        ,"  dt            - dumps simple thread info of all window threads\n"
         "  dt -c         - dumps current thread for current processor.\n"
         "  dt -g         - dump GUI threads only\n"
         "  dt id/pet     - dumps thread with given id or PETHREAD pointer\n"
         "  dt -p pti     - takes a pti instead of a thread id/pet\n"
         "  dt -s [...]   - displays wait state\n"
         "  dt -v         - verbose output\n"
        ,"cgpsv"
        ,STDARGS1)
#endif

#ifdef KERNEL
DOIT(   dtdb
        ,"dtdb [ptdb]                   - Dump Task Database\n"
        ,"  dtdb <ptdb>   - dumps given ptdb.\n"
         "  dtdb          - dumps all dtdbs.\n"
        ,""
        ,STDARGS1)
#endif

#ifndef KERNEL
DOIT(   dteb
        ,"dteb                          - Displays selected TEB information.\n"
        ,""
        ,""
        ,NOARGS)
#endif // !KERNEL

#ifdef KERNEL
DOIT(   dti
        ,"dti [pti]                     - Displays THREADINFO structure\n"
        ,"  dti            - dumps THREADINFO structure of foreground thread\n"
         "  dti address    - dumps THREADINFO structure at address\n"
        ,""
        ,STDARGS1)
#endif // KERNEL

#ifdef KERNEL
DOIT(   dtl
        ,"dtl [-t] [pointer|handle]     - Displays thread locks\n"
        ,"dtl pObj      - dump thread locks for object/pool/kernel-object\n"
         "dtl -t pti    - dump thread locks for thread\n"
         "dtl           - dump all thread locks\n"
        ,"t"
        ,STDARGS1)
#endif //KERNEL

#ifdef KERNEL
DOIT(   dtmr
        ,"dtmr [ptmr]                   - Dumps timer structure.\n"
        ,"  dtmr <ptrm> - dump timer.\n"
         "  dtmr        - dump all timers.\n"
        ,""
        ,STDARGS1)
#endif // KERNEL

DOIT(   du
        ,"du [pointer|handle]           - Generic object dumping routine\n"
        ,"  Attempts to dump the object appropriately.\n"
         "  If no object is given - WARNING - ALL objects are dumped.\n"
        ,""
        ,STDARGS1)

#ifdef KERNEL
DOIT(   dup
        ,"dup                           - User Preferences DWORDS\n"
        ,""
        ,""
        ,NOARGS)
#endif //KERNEL

#ifdef KERNEL
DOIT(   dupm
        ,"dupm                          - User Preference Bitmask\n"
        ,""
        ,""
        ,NOARGS)
#endif //KERNEL

#ifdef KERNEL
DOIT(   dumphmgr
        ,"dumphmgr [-s]                 - Dumps object allocation counts (dbg only)\n"
        ,""
        ,"s"
        ,STDARGS0)
#endif //KERNEL

DOIT(   dw
        ,"dw -aefhvsprwoz [hwnd/pwnd]   - Displays information on windows in system\n"
        ,
         "  dw hwnd/pwnd  - dumps simple window info for window at pwnd\n"
         "  dw            - dumps simple window info for all top level windows of current\n"
         "                  desktop.\n"
         "  dw -v [pwnd]  - dumps verbose window info\n"
         "  dw -a         - added to any of the other commands - applies to all desktops.\n"
         "  dw -f [pwnd]  - dumps flags for window at pwnd or all toplevel windows\n"
         "  dw -h         - add -h to show wndproc symbol - slower.\n"
         "  dw -o pwnd    - dumps properties of pwnd.\n"
         "  dw -p pwnd    - dumps info for all child windows of window at pwnd\n"
         "  dw -r [pwnd]  - dumps relationship of windows beneath pwnd or desktop window\n"
         "  dw -s pwnd    - dumps info for all sibling windows of window at pwnd\n"
         "  dw -t <pti>   - Like EnumThreadWindows\n"
         "  dw -w pwnd    - dumps window words of pwnd.\n"
         "  dw -z         - Added to show z-ordering/activation revelant info\n"
        ,"afhvsprtwoz"
        ,STDARGS1)

#ifdef KERNEL
DOIT(   dwe
        ,"dwe [-n] [addr]               - Displays WinEvent hooks/notifies\n"
        ,
         "  dwe <peh>    - dump EVENTHOOK struct.\n"
         "  dwe          - dump all EVENTHOOKS.\n"
         "  dwe -n <pn>  - dump NOTIFY struct.\n"
         "  dwe -n       - dump all NOTIFYs.\n"
        ,"n"
        ,STDARGS1)
#endif // KERNEL

#ifdef KERNEL
DOIT(   dwpi
        ,"dwpi -p [pwpi | ppi]          - Displays WOWPROCESSINFO structure specified\n"
        ,"  dwpi <pwpi>   - dump a pwpi structure.\n"
         "  dwpi -p <ppi> - dump pwpis for this process.\n"
         "  dwpi          - dump all pwpis.\n"
        ,"p"
        ,STDARGS1)
#endif // KERNEL

#ifdef KERNEL
DOIT(   dws
        ,"dws [pws]                     - Dump windowstations\n"
        ,"  dws <pws>     - dump windows station\n"
         "  dws           - dump all window stations\n"
        ,""
        ,STDARGS1)
DOIT(   dcss
        ,"dcss                          - Dump critical section stack traces\n"
        ,"  dws           - dump all stack straces\n"
        ,""
        ,STDARGS1)
DOIT(   dpa
        ,"dpa -cvsfrp     - Dump pool allocations\n"
        ,"  dpa -c        - dump current pool allocations statistics\n"
         "  dpa -v        - dump all the current pool allocations\n"
         "  dpa -vs       - include stack traces\n"
         "  dpa -f        - dump failed allocations\n"
         "  dpa -r        - dump free pool\n"
         "  dpa -p <ptr>  - dump the allocation containing pointer 'ptr'\n"
        ,"cvsfrp"
        ,STDARGS1)
DOIT(   dvs
        ,"dvs -s                        - Dump sections and mapped views\n"
        ,"  dvs -s        - dump stack traces also\n"
        ,"s"
        ,STDARGS1)
DOIT(   dfa
        ,"dfa                           - Dump allocation fail stack trace\n"
        ,"  dfa           - dump allocation fail stack trace\n"
        ,""
        ,STDARGS1)
DOIT(   dha
        ,"dha    address  - Dump heap allocations.  Also verifies the heap the address belongs to\n"
        ,"  dha -v        - Prints out all allocations in the heap\n"
         "  dha -a        - apply the command to all heaps.  \nWith no address specified, it checks all allocations in all heaps\n"
        ,"va"
        ,STDARGS1)
#endif //KERNEL

DOIT(   dy
        ,"dy [pdi]                      - Dump DISPLAYINFO\n"
        ,"  dy             - dump gpDispInfo\n"
         "  dy address     - dump DISPLAYINFO at address\n"
        ,""
        ,STDARGS1)

DOIT(   find
        ,"find baseaddr addr [o#]       - Find linked list element\n"
        ,"  find baseaddr addr      - searches list for addr starting at baseaddr,\n"
         "                            assumes link is first DWORD, w/NULL termination.\n"
         "  find baseaddr addr o4   - next link is 4 DWORDS from start of structure\n"
        ,""
        ,CUSTOM)

DOIT(   fno
        ,"fno <address>                 - Find nearest object.\n"
        ,"  Locates the nearest public USER objects to the given address.\n"
        ,""
        ,STDARGS1)

DOIT(   frr
        ,"frr <psrcLo> <psrcHi> <prefLo> [prefHi] - Find Range Reference\n"
        ,"  Used to search memory within the src range for references to\n"
         "  addresses within the ref range.\n"
        ,""
        ,STDARGS4)

#ifdef KERNEL

DOIT(   gflags
        ,"\0gflags [-v]                   - Displays NT Global Flags\n"
        ,"  gflags      - Dump and interpret the NT global flags\n"
         "  gflags -v   - A little bit verbose\n"
        ,"v"
        ,STDARGS0)

#endif

#ifdef KERNEL
DOIT(   kbd
        ,"kbd -au [pq]                  - Displays key state for queue\n"
        ,"  kbd            - key state for foreground queue\n"
         "  kbd pq         - key state for queue structure at pq\n"
         "  kbd -a         - key state for all queues\n"
         "  kbd -u pb      - wParam of WM_UPDATEKEYSTATE event\n"
        ,"au"
        ,STDARGS1)
#endif // KERNEL

#ifndef KERNEL
DOIT(   kbp
        ,"kbp                           - Breaks into the Kernel Debugger\n"
        ,""
        ,""
        ,NOARGS)
#endif

DOIT(   sas
        ,"sas [-s] <addr> [length]      - Stack Analysis Stuff\n"
        ,"sas -d [-s] <number> - to just analyze a number.\n"
         " length defaults to 25.\n"
         " -s suppresses symbolic output.\n"
        ,"ds"
        ,STDARGS2)

#if DEBUGTAGS
DOIT(   tag
        ,"tag [<tag> [<flags>]]         - Displays or sets tags\n"
        ,"  tag               - display all tags\n"
         "  tag <tag>         - display one tag\n"
         "  tag <tag> <flags> - change tag settings\n"
         "       <flags> = [0-3] Disabled = 0, Enabled = 1, Print = 2, Prompt = 3\n"
        ,""
        ,CUSTOM)
#endif

DOIT(   test
        ,"test                          - Test basic debug functions.\n"
        ,""
        ,""
        ,NOARGS)

DOIT(   uver
        ,"uver                          - show versions of user and exts.\n"
        ,""
        ,""
        ,NOARGS)

DOIT(   vkey
        ,""
        ,"vkey                          - show vkey values\n"
         "  vkey hex          - display VK symbol\n"
         "  vkey VK_*         - display hex value for VK_ symbol\n"
         "  vkey -a           - display all common VK values\n"
         "  vkey -o           - display all common VK values in one column\n"
        ,"ao"
        ,CUSTOM)

#ifdef KERNEL

DOIT(   hh
        ,"hh                            - dumps gdwHydraHint.\n"
        ,"hh hydrahint        - dumps a specified DWORD as a hydra hint"
        ,""
        ,STDARGS1)
#endif // KERNEL

#ifndef KERNEL
DOIT(   kc
        ,"kc pwnd                       - Displays information on Keyboard Cues\n"
        ,""
        ,""
        ,STDARGS1)
#endif

DOIT(   wm
        ,"wm, vkey, ddlgt, disi: see each help\n"
        ,"wm <msg>                      - Displays window message symbol\n"
         "  wm hex            - display WM_ symb\n"
         "  wm WM_            - display message value\n"
         "  wm -a             - display all common window messages\n"
        ,"a"
        ,CUSTOM)


