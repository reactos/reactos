<group>
<module name="msimtf" type="win32dll" baseaddress="${BASEADDRESS_MSIMTF}" installbase="system32" installname="msimtf.dll">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="msimtf.spec" />
	<include base="msimtf">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>activeimmapp.c</file>
	<file>main.c</file>
	<file>rsrc.rc</file>
	<library>wine</library>
	<library>uuid</library>
	<library>imm32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>