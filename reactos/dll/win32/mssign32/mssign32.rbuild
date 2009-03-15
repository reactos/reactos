<module name="mssign32" type="win32dll" baseaddress="${BASEADDRESS_MSSIGN32}" installbase="system32" installname="mssign32.dll" allowwarnings="true">
	<!--autoregister infsection="OleControlDlls" type="DllRegisterServer" /-->
	<importlibrary definition="mssign32.spec" />
	<include base="mssign32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>mssign32_main.c</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
