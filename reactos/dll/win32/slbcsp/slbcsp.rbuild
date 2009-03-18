<module name="slbcsp" type="win32dll" installbase="system32" installname="slbcsp.dll" allowwarnings="true">
	<!--autoregister infsection="OleControlDlls" type="DllRegisterServer" /-->
	<importlibrary definition="slbcsp.spec" />
	<include base="slbcsp">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>main.c</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
