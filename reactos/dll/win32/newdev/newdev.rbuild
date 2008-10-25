<module name="newdev" type="win32dll" installbase="system32" installname="newdev.dll" unicode="true">
	<include base="newdev">.</include>
	<importlibrary definition="newdev.spec" />
	<file>newdev.c</file>
	<file>stubs.c</file>
	<file>wizard.c</file>
	<file>newdev.rc</file>
	<file>newdev.spec</file>
	<library>wine</library>
	<library>gdi32</library>
	<library>comctl32</library>
	<library>ntdll</library>
	<library>setupapi</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>shell32</library>
</module>
