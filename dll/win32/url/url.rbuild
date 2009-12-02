<module name="url" type="win32dll" baseaddress="${BASEADDRESS_URL}" installbase="system32" installname="url.dll" allowwarnings="true">
	<importlibrary definition="url.spec" />
	<include base="url">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>url_main.c</file>
	<library>wine</library>
	<library>shell32</library>
	<library>shlwapi</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
