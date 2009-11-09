<module name="resutils" type="win32dll" baseaddress="${BASEADDRESS_RESUTILS}" installbase="system32" installname="resutils.dll" allowwarnings="true">
	<importlibrary definition="resutils.spec" />
	<include base="resutils">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>resutils.c</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
