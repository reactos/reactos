<module name="reg" type="win32cui" installbase="system32" installname="reg.exe" unicode="true">
	<include base="reg">.</include>
	<redefine name="_WIN32_WINNT">0x600</redefine>
	<library>wine</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>kernel32</library>
	<file>reg.c</file>
	<file>rsrc.rc</file>
</module>
