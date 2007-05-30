<module name="comdlg32" type="win32dll" baseaddress="${BASEADDRESS_COMDLG32}" installbase="system32" installname="comdlg32.dll" allowwarnings="true" usewrc="false">
	<importlibrary definition="comdlg32.spec.def" />
	<include base="comdlg32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<library>wine</library>
	<library>uuid</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>gdi32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>shell32</library>
	<library>shlwapi</library>
	<library>winspool</library>
	<library>ole32</library>
	<file>cdlg32.c</file>
	<file>colordlg.c</file>
	<file>filedlg.c</file>
	<file>filedlg31.c</file>
	<file>filedlgbrowser.c</file>
	<file>filetitle.c</file>
	<file>finddlg32.c</file>
	<file>fontdlg.c</file>
	<file>printdlg.c</file>
	<file>rsrc.rc</file>
	<file>comdlg32.spec</file>
	<pch>precomp.h</pch>
</module>
