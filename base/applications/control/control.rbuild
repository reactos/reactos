<module name="control" type="win32gui" baseaddress="${BASEADDRESS_CONTROL}" installbase="system32" installname="control.exe" unicode="yes">
	<include base="control">.</include>
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>comctl32</library>
	<file>control.c</file>
	<file>control.rc</file>
</module>
