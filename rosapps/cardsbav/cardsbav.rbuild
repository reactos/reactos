<module name="cardsbav" type="win32dll" baseaddress="${BASEADDRESS_CARDS}" installbase="system32" installname="cardsbav.dll" unicode="yes">
	<importlibrary definition="cardsbav.def" />
	<include base="cardsbav">.</include>
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>gdi32</library>
	<library>user32</library>
	<file>cardsbav.c</file>
	<file>cardsbav.rc</file>
</module>
