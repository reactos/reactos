<module name="packmgr" type="win32gui" installbase="system32" installname="packmgr.exe">
	<include base="package">.</include>
	<include base="packmgr">.</include>
	<define name="__USE_W32API" />
	<define name="UNICODE" />
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>

	<library>kernel32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>package</library>

	<file>main.c</file>
	<file>packmgr.rc</file>
</module>
