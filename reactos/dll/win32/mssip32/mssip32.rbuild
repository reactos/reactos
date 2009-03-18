<module name="mssip32" type="win32dll" baseaddress="${BASEADDRESS_MSSIP32}" installbase="system32" installname="mssip32.dll" allowwarnings="true">
	<!--autoregister infsection="OleControlDlls" type="DllRegisterServer" /-->
	<importlibrary definition="mssip32.spec" />
	<include base="mssip32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>main.c</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
