<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="urlmon" type="win32dll" baseaddress="${BASEADDRESS_URLMON}" installbase="system32" installname="urlmon.dll">
	<autoregister infsection="OleControlDlls" type="Both" />
	<importlibrary definition="urlmon.spec" />
	<include base="urlmon">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="_WIN32_WINNT">0x600</define>
	<file>bindctx.c</file>
	<file>binding.c</file>
	<file>bindprot.c</file>
	<file>download.c</file>
	<file>file.c</file>
	<file>format.c</file>
	<file>ftp.c</file>
	<file>http.c</file>
	<file>internet.c</file>
	<file>mk.c</file>
	<file>regsvr.c</file>
	<file>sec_mgr.c</file>
	<file>session.c</file>
	<file>umon.c</file>
	<file>umstream.c</file>
	<file>urlmon_main.c</file>
	<file>rsrc.rc</file>
	<library>wine</library>
	<library>uuid</library>
	<library>ole32</library>
	<library>shlwapi</library>
	<library>wininet</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
