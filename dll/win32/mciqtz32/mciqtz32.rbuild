<module name="mciqtz32" type="win32dll" baseaddress="${BASEADDRESS_MCIQTZ32}" installbase="system32" installname="mciqtz32.dll" allowwarnings="true">
	<importlibrary definition="mciqtz32.spec" />
	<include base="mciqtz32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>mciqtz.c</file>
	<file>version.rc</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>oleaut32</library>
	<library>ole32</library>
	<library>winmm</library>
	<library>user32</library>
	<library>ntdll</library>
	<library>strmiids</library>
</module>
