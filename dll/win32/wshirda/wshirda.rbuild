<module name="wshirda" type="win32dll" baseaddress="${BASEADDRESS_WSHIRDA}" installbase="system32" installname="wshirda.dll">
	<importlibrary definition="wshirda.def" />
	<include base="wshirda">.</include>
	<define name="UNICODE" />
	<define name="_DISABLE_TIDENTS" />
	<library>ntdll</library>
	<library>kernel32</library>
	<library>ws2_32</library>
	<file>wshirda.c</file>
	<file>wshirda.rc</file>
</module>
