<module name="query" type="win32dll" baseaddress="${BASEADDRESS_QUERY}" installbase="system32" installname="query.dll" allowwarnings="true">
	<!--autoregister infsection="OleControlDlls" type="DllRegisterServer" /-->
	<importlibrary definition="query.spec" />
	<include base="query">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>query_main.c</file>
	<library>wine</library>
	<library>uuid</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
