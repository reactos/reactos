<module name="mscms" type="win32dll" baseaddress="${BASEADDRESS_MSCMS}" installbase="system32" installname="mscms.dll" allowwarnings="true">
	<importlibrary definition="mscms.spec" />
	<include base="mscms">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>handle.c</file>
	<file>icc.c</file>
	<file>mscms_main.c</file>
	<file>profile.c</file>
	<file>stub.c</file>
	<file>transform.c</file>
	<file>version.rc</file>
	<library>wine</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
