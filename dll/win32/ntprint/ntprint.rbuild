<group>
<module name="ntprint" type="win32dll" baseaddress="${BASEADDRESS_NTPRINT}" installbase="system32" installname="ntprint.dll" allowwarnings="true">
	<importlibrary definition="ntprint.spec" />
	<include base="ntprint">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>ntprint.c</file>
	<file>ntprint.rc</file>
	<library>wine</library>
	<library>winspool</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
