<module name="access" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_ACCESS}"  installbase="system32" installname="access.cpl">
	<importlibrary definition="access.def" />
	<include base="access">.</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>msvcrt</library>
	<file>access.c</file>
	<file>display.c</file>
	<file>general.c</file>
	<file>keyboard.c</file>
	<file>mouse.c</file>
	<file>sound.c</file>
	<file>access.rc</file>
</module>
