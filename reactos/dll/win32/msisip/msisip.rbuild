<module name="msisip" type="win32dll" baseaddress="${BASEADDRESS_MSISIP}" installbase="system32" installname="msisip.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="msisip.spec" />
	<include base="msisip">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>main.c</file>
	<library>wine</library>
	<library>crypt32</library>
	<library>ole32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
