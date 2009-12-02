<module name="msvidc32" type="win32dll" baseaddress="${BASEADDRESS_MSVIDC32}" installbase="system32" installname="msvidc32.dll" allowwarnings="true">
	<importlibrary definition="msvidc32.spec" />
	<include base="msvidc32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>msvideo1.c</file>
	<file>rsrc.rc</file>
	<library>wine</library>
	<library>user32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
