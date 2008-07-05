<module name="gdihv" type="win32gui" installbase="system32" installname="gdihv.exe">
	<include base="gdihv">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>user32</library>
	<library>kernel32</library>
	<library>comctl32</library>
	<library>psapi</library>
	<file>gdihv.c</file>
	<file>gdihv.rc</file>
	<file>mainwnd.c</file>
	<file>handlelist.c</file>
	<file>proclist.c</file>
</module>
