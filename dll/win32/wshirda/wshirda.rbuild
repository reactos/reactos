<module name="wshirda" type="win32dll" baseaddress="${BASEADDRESS_WSHIRDA}" installbase="system32" installname="wshirda.dll" unicode="yes">
	<importlibrary definition="wshirda.spec" />
	<include base="wshirda">.</include>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>ws2_32</library>
	<file>wshirda.c</file>
	<file>wshirda.rc</file>
</module>
