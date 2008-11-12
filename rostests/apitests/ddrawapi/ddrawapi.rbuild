<module name="ddrawapi" type="win32cui">
	<include base="ddrawapi">.</include>
	<define name="__USE_W32API" />
	<library>apitest</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>d3d8thk</library>
	<library>dxguid</library>
	<library>ddraw</library>
	<library>ole32</library>
	<library>advapi32</library>
	<library>shell32</library>		
	<file>ddrawapi.c</file>
	<file>testlist.c</file>
</module>
