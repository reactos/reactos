<module name="template_mdi" type="win32cui" installname="mdi.exe" allowwarnings="true">
	<include base="template_mdi">.</include>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
 	<file>about.c</file>
        <file>framewnd.c</file>
        <file>childwnd.c</file>
        <file>main.c</file>
</module>
