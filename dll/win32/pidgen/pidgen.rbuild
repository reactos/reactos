<module name="pidgen" type="win32dll" baseaddress="${BASEADDRESS_PIDGEN}" installbase="system32" installname="pidgen.dll" allowwarnings="true">
	<importlibrary definition="pidgen.spec" />
	<include base="pidgen">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>main.c</file>
	<file>rsrc.rc</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
