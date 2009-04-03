<module name="msdmo" type="win32dll" baseaddress="${BASEADDRESS_MSDMO}" installbase="system32" installname="msdmo.dll" allowwarnings="true" entrypoint="0">
	<importlibrary definition="msdmo.spec" />
	<include base="msdmo">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>dmoreg.c</file>
	<file>dmort.c</file>
	<file>rsrc.rc</file>
	<library>wine</library>
	<library>uuid</library>
	<library>ole32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
