<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="mingw_common" type="staticlibrary" isstartuplib="true" underscoresymbols="true" crt="dll">
	<importlibrary definition="moldname-msvcrt.def" dllname="msvcrt.dll" />
	<include base="mingw_common">include</include>
	<file>_newmode.c</file>
	<file>_wgetopt.c</file>
	<file>argv.c</file>
	<file>atonexit.c</file>
	<file>binmode.c</file>
	<file>charmax.c</file>
	<file>cinitexe.c</file>
	<file>CRT_fp10.c</file>
	<file>CRT_fp8.c</file>
	<file>dllentry.c</file>
	<file>gccmain.c</file>
	<file>getopt.c</file>
	<file>gs_support.c</file>
	<file>merr.c</file>
	<file>mingw_helpers.c</file>
	<file>natstart.c</file>
	<file>pesect.c</file>
	<file>pseudo-reloc.c</file>
	<file>pseudo-reloc-list.c</file>
	<file>tlssup.c</file>
	<file>wildcard.c</file>
	<file>xncommod.c</file>
	<file>xthdloc.c</file>
	<file>xtxtmode.c</file>
</module>
<module name="mingw_main" type="staticlibrary" isstartuplib="true" allowwarnings="true" crt="dll">
	<include base="mingw_common">include</include>
	<file>crt0_c.c</file>
	<file>crtexe.c</file>
	<file>dllargv.c</file>
</module>
<module name="mingw_wmain" type="staticlibrary" isstartuplib="true" allowwarnings="true" unicode="yes" crt="dll">
	<include base="mingw_common">include</include>
	<define name="WPRFLAG"/>
	<file>crt0_w.c</file>
	<file>crtexe.c</file>
	<file>dllargv.c</file>
</module>
<module name="mingw_dllmain" type="staticlibrary" isstartuplib="true" crt="dll">
	<include base="mingw_common">include</include>
	<file>dllargv.c</file>
	<file>crtdll.c</file>
</module>
</group>
