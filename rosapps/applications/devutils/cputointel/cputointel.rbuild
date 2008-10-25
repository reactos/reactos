<module name="cputointel" type="win32cui" installbase="system32" installname="cputointel.exe"  stdlib="host">
	<include base="cputointel">.</include>
	<library>kernel32</library>
	<library>user32</library>

	<file>CpuToIntel.c</file>
	<file>misc.c</file>

	<file>From/ARM/ARMBrain.c</file>
	<file>From/ARM/ARMopcode.c</file>

	<file>From/IA32/IA32Brain.c</file>
	<file>From/IA32/IA32opcode.c</file>

	<file>From/m68k/M68kBrain.c</file>
	<file>From/m68k/M68kopcode.c</file>

	<file>From/PPC/PPCBrain.c</file>
	<file>From/PPC/PPCopcode.c</file>

	<file>From/dummycpu/DummyBrain.c</file>
	<file>From/dummycpu/Dummyopcode.c</file>

	<file>ImageLoader.c</file>
	<file>AnyalsingProcess.c</file>
	<file>ConvertingProcess.c</file>
	<file>ConvertToIA32Process.c</file>
	<file>ConvertToPPCProcess.c</file>


</module>