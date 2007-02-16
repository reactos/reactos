<module name="winver" type="win32gui" installbase="system32" installname="winver.exe" usewrc="false">
	<include base="winver">.</include>
	<define name="__USE_W32API" />
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="_WIN32_IE">0x501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>shell32</library>
	<library>kernel32</library>
	<file>winver.c</file>	
</module>
