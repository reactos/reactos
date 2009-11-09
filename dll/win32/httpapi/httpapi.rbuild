<module name="httpapi" type="win32dll" baseaddress="${BASEADDRESS_HTTPAPI}" installbase="system32" installname="httpapi.dll" allowwarnings="true">
	<importlibrary definition="httpapi.spec" />
	<include base="httpapi">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>httpapi_main.c</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
