<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="shdocvw" type="win32dll" baseaddress="${BASEADDRESS_SHDOCVW}" installbase="system32" installname="shdocvw.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="Both" />
	<importlibrary definition="shdocvw.spec" />
	<include base="shdocvw">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<include base="shdocvw" root="intermediate">.</include>
	<define name="_SHDOCVW_" />
	<library>wine</library>
	<library>uuid</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>ole32</library>
	<library>oleaut32</library>
	<library>gdi32</library>
	<library>shell32</library>
	<library>shlwapi</library>
	<library>version</library>
	<library>urlmon</library>
	<dependency>shdocvw_v1</dependency>
	<file>classinfo.c</file>
	<file>client.c</file>
	<file>dochost.c</file>
	<file>events.c</file>
	<file>factory.c</file>
	<file>frame.c</file>
	<file>ie.c</file>
	<file>iexplore.c</file>
	<file>intshcut.c</file>
	<file>navigate.c</file>
	<file>oleobject.c</file>
	<file>persist.c</file>
	<file>shdocvw_main.c</file>
	<file>shlinstobj.c</file>
	<file>urlhist.c</file>
	<file>view.c</file>
	<file>webbrowser.c</file>
	<file>shdocvw.rc</file>
</module>
<module name="shdocvw_v1" type="embeddedtypelib" allowwarnings="true">
	<file>shdocvw_v1.idl</file>
</module>
</group>