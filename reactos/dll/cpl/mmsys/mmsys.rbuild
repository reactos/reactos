<module name="mmsys" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_MMSYS}" installbase="system32" installname="mmsys.cpl">
	<importlibrary definition="mmsys.def" />
	<include base="mmsys">.</include>
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
	<library>devmgr</library>
	<file>mmsys.c</file>
	<file>mmsys.rc</file>
</module>
