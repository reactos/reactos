<module name="msisys" type="win32ocx" installbase="system32" installname="msisys.ocx" allowwarnings="true">
	<!--autoregister infsection="OleControlDlls" type="DllRegisterServer" /-->
	<importlibrary definition="msisys.ocx.spec" />
	<include base="msisys">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>msisys.c</file>
	<library>wine</library>
	<library>uuid</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
