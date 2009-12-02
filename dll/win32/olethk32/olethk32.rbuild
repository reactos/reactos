<module name="olethk32" type="win32dll" baseaddress="${BASEADDRESS_OLETHK32}" installbase="system32" installname="olethk32.dll" allowwarnings="true">
	<importlibrary definition="olethk32.spec" />
	<include base="olethk32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>main.c</file>
	<file>version.rc</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
