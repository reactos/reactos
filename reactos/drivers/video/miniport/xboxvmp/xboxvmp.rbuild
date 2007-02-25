<module name="xboxvmp" type="kernelmodedriver">
	<include base="xboxvmp">.</include>
	<define name="__USE_W32API" />
	<library>ntoskrnl</library>
	<library>videoprt</library>
	<file>xboxvmp.c</file>
	<file>xboxvmp.rc</file>
</module>
