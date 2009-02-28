<module name="mprapi" type="win32dll" baseaddress="${BASEADDRESS_MPRAPI}" installbase="system32" installname="mprapi.dll">
	<importlibrary definition="mprapi.spec" />
	<include base="mprapi">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<library>wine</library>
	<library>kernel32</library>
	<file>mprapi.c</file>
</module>
