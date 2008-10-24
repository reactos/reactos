<module name="winspool" type="win32dll" extension=".drv" baseaddress="${BASEADDRESS_WINSPOOL}" installbase="system32" installname="winspool.drv" allowwarnings="true" unicode="yes">
	<importlibrary definition="winspool.spec" />
	<include base="winspool">.</include>
	<define name="_DISABLE_TIDENTS" />
	<library>ntdll</library>
	<library>kernel32</library>
	<file>info.c</file>
	<file>stubs.c</file>
	<file>winspool.rc</file>
	<file>winspool.spec</file>
</module>
