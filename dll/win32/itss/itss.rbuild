<group>
<module name="itss" type="win32dll" baseaddress="${BASEADDRESS_ITSS}" installbase="system32" installname="itss.dll" crt="msvcrt">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="itss.spec" />
	<include base="itss">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>chm_lib.c</file>
	<file>lzx.c</file>
	<file>itss.c</file>
	<file>moniker.c</file>
	<file>protocol.c</file>
	<file>storage.c</file>
	<file>rsrc.rc</file>
	<library>wine</library>
	<library>uuid</library>
	<library>urlmon</library>
	<library>shlwapi</library>
	<library>ole32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
