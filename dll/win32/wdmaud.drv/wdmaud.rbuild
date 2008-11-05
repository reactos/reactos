<module name="wdmaud.drv" type="win32dll" baseaddress="${BASEADDRESS_WDMAUD}" installbase="system32" installname="wdmaud.drv">
	<importlibrary definition="wdmaud.spec" />
	<include base="wdmaud.drv">.</include>
	<define name="_DISABLE_TIDENTS" />
	<library>advapi32</library>
	<library>kernel32</library>
	<library>winmm</library>
	<library>user32</library>
	<library>winmm</library>
	<file>wdmaud.c</file>
	<file>wdmaud.rc</file>
</module>
