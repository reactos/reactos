<module name="faultrep" type="win32dll" baseaddress="${BASEADDRESS_FAULTREP}" installbase="system32" installname="faultrep.dll">
	<importlibrary definition="faultrep.spec" />
	<include base="fusion">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<library>wine</library>
	<library>advapi32</library>
	<file>faultrep.c</file>
</module>
