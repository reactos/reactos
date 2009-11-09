<module name="iccvid" type="win32dll" baseaddress="${BASEADDRESS_ICCVID}" installbase="system32" installname="iccvid.dll" allowwarnings="true">
	<importlibrary definition="iccvid.spec" />
	<include base="iccvid">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>iccvid.c</file>
	<file>rsrc.rc</file>
	<library>wine</library>
	<library>user32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>