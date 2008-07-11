# Microsoft Developer Studio Project File - Name="Ext3Fsd" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=Ext3Fsd - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ext3fsd.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ext3fsd.mak" CFG="Ext3Fsd - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Ext3Fsd - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "Ext3Fsd - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "Ext3Fsd"
# PROP Scc_LocalPath "."

!IF  "$(CFG)" == "Ext3Fsd - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "fre"
# PROP BASE Intermediate_Dir "fre"
# PROP BASE Cmd_Line "NMAKE /f Ext3Fsd.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Ext3Fsd.exe"
# PROP BASE Bsc_Name "Ext2Fsd.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "fre"
# PROP Intermediate_Dir "fre"
# PROP Cmd_Line ".\DDKBuild.bat -WNET chk ."
# PROP Rebuild_Opt "-cZ"
# PROP Target_File "Ext3Fsd.sys"
# PROP Bsc_Name ".\winnet\chk\i386\Ext2Fsd.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "Ext3Fsd - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "chk"
# PROP BASE Intermediate_Dir "chk"
# PROP BASE Cmd_Line "NMAKE /f Ext3Fsd.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Ext3Fsd.exe"
# PROP BASE Bsc_Name "Ext2Fsd.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "chk"
# PROP Intermediate_Dir "chk"
# PROP Cmd_Line ".\DDKBuild.bat -WNET chk ."
# PROP Rebuild_Opt "-ceZ"
# PROP Target_File "Ext2Fsd.sys"
# PROP Bsc_Name ".\winnet\chk\i386\Ext2Fsd.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "Ext3Fsd - Win32 Release"
# Name "Ext3Fsd - Win32 Debug"

!IF  "$(CFG)" == "Ext3Fsd - Win32 Release"

!ELSEIF  "$(CFG)" == "Ext3Fsd - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "nls"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\nls\MAKEFILE
# End Source File
# Begin Source File

SOURCE=.\nls\nls_ascii.c
# End Source File
# Begin Source File

SOURCE=.\nls\nls_base.c
# End Source File
# Begin Source File

SOURCE=.\nls\nls_cp1250.c
# End Source File
# Begin Source File

SOURCE=.\nls\nls_cp1251.c
# End Source File
# Begin Source File

SOURCE=.\nls\nls_cp1255.c
# End Source File
# Begin Source File

SOURCE=.\nls\nls_cp437.c
# End Source File
# Begin Source File

SOURCE=.\nls\nls_cp737.c
# End Source File
# Begin Source File

SOURCE=.\nls\nls_cp775.c
# End Source File
# Begin Source File

SOURCE=.\nls\nls_cp850.c
# End Source File
# Begin Source File

SOURCE=.\nls\nls_cp852.c
# End Source File
# Begin Source File

SOURCE=.\nls\nls_cp855.c
# End Source File
# Begin Source File

SOURCE=.\nls\nls_cp857.c
# End Source File
# Begin Source File

SOURCE=.\nls\nls_cp860.c
# End Source File
# Begin Source File

SOURCE=.\nls\nls_cp861.c
# End Source File
# Begin Source File

SOURCE=.\nls\nls_cp862.c
# End Source File
# Begin Source File

SOURCE=.\nls\nls_cp863.c
# End Source File
# Begin Source File

SOURCE=.\nls\nls_cp864.c
# End Source File
# Begin Source File

SOURCE=.\nls\nls_cp865.c
# End Source File
# Begin Source File

SOURCE=.\nls\nls_cp866.c
# End Source File
# Begin Source File

SOURCE=.\nls\nls_cp869.c
# End Source File
# Begin Source File

SOURCE=.\nls\nls_cp874.c
# End Source File
# Begin Source File

SOURCE=.\nls\nls_cp932.c
# End Source File
# Begin Source File

SOURCE=.\nls\nls_cp936.c
# End Source File
# Begin Source File

SOURCE=.\nls\nls_cp949.c
# End Source File
# Begin Source File

SOURCE=.\nls\nls_cp950.c
# End Source File
# Begin Source File

SOURCE=".\nls\nls_euc-jp.c"
# End Source File
# Begin Source File

SOURCE=".\nls\nls_iso8859-1.c"
# End Source File
# Begin Source File

SOURCE=".\nls\nls_iso8859-13.c"
# End Source File
# Begin Source File

SOURCE=".\nls\nls_iso8859-14.c"
# End Source File
# Begin Source File

SOURCE=".\nls\nls_iso8859-15.c"
# End Source File
# Begin Source File

SOURCE=".\nls\nls_iso8859-2.c"
# End Source File
# Begin Source File

SOURCE=".\nls\nls_iso8859-3.c"
# End Source File
# Begin Source File

SOURCE=".\nls\nls_iso8859-4.c"
# End Source File
# Begin Source File

SOURCE=".\nls\nls_iso8859-5.c"
# End Source File
# Begin Source File

SOURCE=".\nls\nls_iso8859-6.c"
# End Source File
# Begin Source File

SOURCE=".\nls\nls_iso8859-7.c"
# End Source File
# Begin Source File

SOURCE=".\nls\nls_iso8859-9.c"
# End Source File
# Begin Source File

SOURCE=".\nls\nls_koi8-r.c"
# End Source File
# Begin Source File

SOURCE=".\nls\nls_koi8-ru.c"
# End Source File
# Begin Source File

SOURCE=".\nls\nls_koi8-u.c"
# End Source File
# Begin Source File

SOURCE=.\nls\nls_utf8.c
# End Source File
# Begin Source File

SOURCE=.\nls\Sources
# End Source File
# End Group
# Begin Group "ext3"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ext3\generic.c
# End Source File
# Begin Source File

SOURCE=.\ext3\MAKEFILE
# End Source File
# Begin Source File

SOURCE=.\ext3\recover.c
# End Source File
# Begin Source File

SOURCE=.\ext3\Sources
# End Source File
# End Group
# Begin Group "jbd"

# PROP Default_Filter ""
# End Group
# Begin Source File

SOURCE=.\block.c
# End Source File
# Begin Source File

SOURCE=.\cleanup.c
# End Source File
# Begin Source File

SOURCE=.\close.c
# End Source File
# Begin Source File

SOURCE=.\cmcb.c
# End Source File
# Begin Source File

SOURCE=.\create.c
# End Source File
# Begin Source File

SOURCE=.\debug.c
# End Source File
# Begin Source File

SOURCE=.\devctl.c
# End Source File
# Begin Source File

SOURCE=.\dirctl.c
# End Source File
# Begin Source File

SOURCE=.\dispatch.c
# End Source File
# Begin Source File

SOURCE=.\except.c
# End Source File
# Begin Source File

SOURCE=.\fastio.c
# End Source File
# Begin Source File

SOURCE=.\fileinfo.c
# End Source File
# Begin Source File

SOURCE=.\flush.c
# End Source File
# Begin Source File

SOURCE=.\fsctl.c
# End Source File
# Begin Source File

SOURCE=.\init.c
# End Source File
# Begin Source File

SOURCE=.\linux.c
# End Source File
# Begin Source File

SOURCE=.\lock.c
# End Source File
# Begin Source File

SOURCE=.\memory.c
# End Source File
# Begin Source File

SOURCE=.\misc.c
# End Source File
# Begin Source File

SOURCE=.\nls.c
# End Source File
# Begin Source File

SOURCE=.\pnp.c
# End Source File
# Begin Source File

SOURCE=.\read.c
# End Source File
# Begin Source File

SOURCE=.\shutdown.c
# End Source File
# Begin Source File

SOURCE=.\volinfo.c
# End Source File
# Begin Source File

SOURCE=.\write.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "Linux"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\include\linux\bitops.h
# End Source File
# Begin Source File

SOURCE=.\include\linux\Ext2_fs.h
# End Source File
# Begin Source File

SOURCE=.\include\linux\ext3_fs.h
# End Source File
# Begin Source File

SOURCE=.\include\linux\ext3_fs_i.h
# End Source File
# Begin Source File

SOURCE=.\include\linux\ext3_fs_sb.h
# End Source File
# Begin Source File

SOURCE=.\include\linux\ext3_jbd.h
# End Source File
# Begin Source File

SOURCE=.\include\linux\fs.h
# End Source File
# Begin Source File

SOURCE=.\include\linux\jbd.h
# End Source File
# Begin Source File

SOURCE=.\include\linux\list.h
# End Source File
# Begin Source File

SOURCE=.\include\linux\module.h
# End Source File
# Begin Source File

SOURCE=.\include\linux\nls.h
# End Source File
# Begin Source File

SOURCE=.\include\linux\types.h
# End Source File
# End Group
# Begin Group "asm"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\include\asm\page.h
# End Source File
# Begin Source File

SOURCE=.\include\asm\semaphore.h
# End Source File
# Begin Source File

SOURCE=.\include\asm\uaccess.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\include\common.h
# End Source File
# Begin Source File

SOURCE=.\include\ext2fs.h
# End Source File
# Begin Source File

SOURCE=.\include\ntifs.gnu.h
# End Source File
# Begin Source File

SOURCE=.\include\resource.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\Ext3Fsd.rc
# End Source File
# Begin Source File

SOURCE=.\sys\MAKEFILE
# End Source File
# Begin Source File

SOURCE=.\sys\Sources
# End Source File
# End Group
# Begin Group "Douments"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\COPYRIGHT.TXT
# End Source File
# Begin Source File

SOURCE=.\FAQ.txt
# End Source File
# Begin Source File

SOURCE=.\notes.txt
# End Source File
# Begin Source File

SOURCE=.\readme.txt
# End Source File
# Begin Source File

SOURCE=.\TODO.txt
# End Source File
# End Group
# End Target
# End Project
