<module name="cputointel" type="win32cui" installbase="system32" installname="cputointel.exe"  stdlib="host">
	<include base="cputointel">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>kernel32</library>
	<library>user32</library>

	<file>CpuToIntel.c</file>
	<file>misc.c</file>

	<file>ARM/ARMBrain.c</file>
	<file>ARM/ARMopcode.c</file>

	<file>m68k/M68kBrain.c</file>
	<file>m68k/M68kopcode.c</file>

	<file>PPC/PPCBrain.c</file>
	<file>PPC/PPCopcode.c</file>

	<file>dummycpu/DummyBrain.c</file>
	<file>dummycpu/Dummyopcode.c</file>

</module>