<module name="traffic" type="win32dll" baseaddress="${BASEADDRESS_TRAFFIC}" installbase="system32" installname="traffic.dll" allowwarnings="true">
	<importlibrary definition="traffic.spec" />
	<include base="traffic">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>traffic_main.c</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
