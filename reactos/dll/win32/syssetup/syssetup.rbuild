<module name="syssetup" type="win32dll" baseaddress="${BASEADDRESS_SYSSETUP}" installbase="system32" installname="syssetup.dll" unicode="yes" allowwarnings="true">
	<importlibrary definition="syssetup.def" />
	<include base="syssetup">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0600</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<define name="_SETUPAPI_VER">0x0501</define>
	<library>pseh</library>
	<library>uuid</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>gdi32</library>
	<library>user32</library>
	<library>samlib</library>
	<library>userenv</library>
	<library>comctl32</library>
	<library>setupapi</library>
	<library>ole32</library>
	<library>shell32</library>
	<library>shlwapi</library>
	<file>classinst.c</file>
	<file>dllmain.c</file>
	<file>install.c</file>
	<file>logfile.c</file>
	<file>wizard.c</file>
	<file>syssetup.rc</file>
</module>
