<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="mingw_common" type="staticlibrary" isstartuplib="true" underscoresymbols="true">
	<importlibrary definition="moldname-msvcrt.def" dllname="msvcrt.dll" />
	<include base="mingw_common">include</include>
	<file>CRT_fp10.c</file>
	<file>atonexit.c</file>
	<file>charmax.c</file>
	<file>cinitexe.c</file>
	<file>gccmain.c</file>
	<file>gs_support.c</file>
	<file>merr.c</file>
	<file>mingw_helpers.c</file>
	<file>natstart.c</file>
	<file>_newmode.c</file>
	<file>pesect.c</file>
	<file>wildcard.c</file>
	<file>xtxtmode.c</file>
	<file>xncommod.c</file>
	<file>pseudo-reloc.c</file>
</module>
<module name="mingw_main" type="staticlibrary" isstartuplib="true" allowwarnings="true">
	<include base="mingw_common">include</include>
	<file>crt0_c.c</file>
	<file>crtexe.c</file>
</module>
<module name="mingw_wmain" type="staticlibrary" isstartuplib="true" allowwarnings="true">
	<include base="mingw_common">include</include>
	<define name="WPRFLAG"/>
	<define name="UNICODE"/>
	<file>crt0_w.c</file>
	<file>crtexe.c</file>
</module>

<module name="mingw_dllmain" type="staticlibrary" isstartuplib="true">
	<include base="mingw_common">include</include>
	<file>dllentry.c</file>
	<file>dllmain.c</file>
</module>
</group>
