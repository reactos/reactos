<module name="winspool" type="win32dll" extension=".drv" baseaddress="${BASEADDRESS_WINSPOOL}" installbase="system32" installname="winspool.drv" allowwarnings="true" unicode="yes">
	<importlibrary definition="winspool.spec" />
	<include base="winspool">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="_DISABLE_TIDENTS" />
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>shlwapi</library>
	<file>info.c</file>
	<file>stubs.c</file>
	<file>winspool.rc</file>
</module>
