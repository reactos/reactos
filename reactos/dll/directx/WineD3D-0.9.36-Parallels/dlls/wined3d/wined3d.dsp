# Microsoft Developer Studio Project File - Name="wined3d" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=wined3d - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "wined3d.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "wined3d.mak" CFG="wined3d - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "wined3d - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "wined3d - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=xicl6.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "wined3d - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /Zi /O2 /I "$(DXSDK_DIR)\include" /I "$(MSSDK)\include" /I "..\..\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "__WINESRC__" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib advapi32.lib uuid.lib dxguid.lib /nologo /subsystem:windows /dll /pdb:"../../bin/wined3d.pdb" /debug /machine:I386 /out:"../../bin/wined3d.dll"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "wined3d - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "$(DXSDK_DIR)\include" /I "$(MSSDK)\include" /I "..\..\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "__WINESRC__" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib advapi32.lib uuid.lib dxguid.lib /nologo /subsystem:windows /dll /pdb:"../../bin/wined3d.pdb" /debug /machine:I386 /out:"../../bin/wined3d.dll" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "wined3d - Win32 Release"
# Name "wined3d - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\arb_program_shader.c
# End Source File
# Begin Source File

SOURCE=.\baseshader.c
# End Source File
# Begin Source File

SOURCE=.\basetexture.c
# End Source File
# Begin Source File

SOURCE=.\context.c
# End Source File
# Begin Source File

SOURCE=.\cubetexture.c
# End Source File
# Begin Source File

SOURCE=.\device.c
# End Source File
# Begin Source File

SOURCE=.\directx.c
# End Source File
# Begin Source File

SOURCE=.\drawprim.c
# End Source File
# Begin Source File

SOURCE=.\glsl_shader.c
# End Source File
# Begin Source File

SOURCE=.\indexbuffer.c
# End Source File
# Begin Source File

SOURCE=.\palette.c
# End Source File
# Begin Source File

SOURCE=.\pglmini.c
# End Source File
# Begin Source File

SOURCE=.\pixelshader.c
# End Source File
# Begin Source File

SOURCE=.\query.c
# End Source File
# Begin Source File

SOURCE=.\resource.c
# End Source File
# Begin Source File

SOURCE=.\state.c
# End Source File
# Begin Source File

SOURCE=.\stateblock.c
# End Source File
# Begin Source File

SOURCE=.\surface.c
# End Source File
# Begin Source File

SOURCE=.\surface_gdi.c
# End Source File
# Begin Source File

SOURCE=.\swapchain.c
# End Source File
# Begin Source File

SOURCE=.\texture.c
# End Source File
# Begin Source File

SOURCE=.\utils.c
# End Source File
# Begin Source File

SOURCE=.\vertexbuffer.c
# End Source File
# Begin Source File

SOURCE=.\vertexdeclaration.c
# End Source File
# Begin Source File

SOURCE=.\vertexshader.c
# End Source File
# Begin Source File

SOURCE=.\volume.c
# End Source File
# Begin Source File

SOURCE=.\volumetexture.c
# End Source File
# Begin Source File

SOURCE=.\wined3d.def
# End Source File
# Begin Source File

SOURCE=.\wined3d_main.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
