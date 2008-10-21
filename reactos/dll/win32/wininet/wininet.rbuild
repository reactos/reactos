<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="wininet" type="win32dll" baseaddress="${BASEADDRESS_WININET}" installbase="system32" installname="wininet.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllInstall" />
	<importlibrary definition="wininet.spec" />
	<include base="wininet">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<library>wine</library>
	<library>mpr</library>
	<library>shlwapi</library>
	<library>shell32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<library>secur32</library>
	<library>crypt32</library>
	<library>ws2_32</library>
	<file>cookie.c</file>
	<file>dialogs.c</file>
	<file>ftp.c</file>
	<file>gopher.c</file>
	<file>http.c</file>
	<file>internet.c</file>
	<file>netconnection.c</file>
	<file>urlcache.c</file>
	<file>utility.c</file>
	<file>wininet_main.c</file>
	<file>rsrc.rc</file>
	<file>version.rc</file>
	<file>wininet.spec</file>
</module>
</group>
