<group>
<module name="mscat32" type="win32dll" baseaddress="${BASEADDRESS_MSCAT32}" installbase="system32" installname="mscat32.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="mscat32.spec" />
	<include base="mscat32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>main.c</file>
	<library>wine</library>
	<library>wintrust</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
