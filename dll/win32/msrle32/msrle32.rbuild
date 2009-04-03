<module name="msrle32" type="win32dll" baseaddress="${BASEADDRESS_MSRLE32}" installbase="system32" installname="msrle32.dll" allowwarnings="true">
	<importlibrary definition="msrle32.spec" />
	<include base="msrle32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>msrle32.c</file>
	<file>rsrc.rc</file>
	<library>wine</library>
	<library>winmm</library>
	<library>user32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
