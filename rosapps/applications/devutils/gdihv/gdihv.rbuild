<module name="gdihv" type="win32gui" installbase="system32" installname="gdihv.exe">
	<include base="gdihv">.</include>
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
