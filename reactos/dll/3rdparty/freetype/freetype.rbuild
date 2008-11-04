<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="freetype" type="kernelmodedll" entrypoint="0" baseaddress="${BASEADDRESS_FREETYPE}" installbase="system32" installname="freetype.dll" allowwarnings="true" crt="libcntpr">
	<importlibrary definition="freetype.def" />
	<include base="freetype">include</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__NTDRIVER__" />
	<define name="__NO_CTYPE_INLINES" />
	<define name="FT2_BUILD_LIBRARY"  />
	<if property="NSWPAT" value="0">
		<define name="TT_CONFIG_OPTION_UNPATENTED_HINTING" />
	</if>
	<if property="NSWPAT" value="1">
		<define name="TT_CONFIG_OPTION_BYTECODE_INTERPRETER" />
	</if>
	<library>ntoskrnl</library>
	<directory name="i386">
		<file>setjmplongjmp.s</file>
	</directory>
	<directory name="src">
		<directory name="base">
			<file>ftsystem.c</file>
			<file>ftinit.c</file>
			<file>ftdebug.c</file>
			<file>_ftbase_ros.c</file>
			<file>ftbbox.c</file>
			<file>ftglyph.c</file>
			<file>ftbdf.c</file>
			<file>ftbitmap.c</file>
			<file>ftgxval.c</file>
			<file>ftmm.c</file>
			<file>ftotval.c</file>
			<file>ftpatent.c</file>
			<file>ftpfr.c</file>
			<file>ftstroke.c</file>
			<file>ftsynth.c</file>
			<file>fttype1.c</file>
			<file>ftwinfnt.c</file>
		</directory>
		<directory name="autofit">
			<file>autofit.c</file>
		</directory>
		<directory name="bdf">
			<file>bdf.c</file>
		</directory>
		<directory name="cache">
			<file>ftcache.c</file>
		</directory>
		<directory name="cff">
			<file>cff.c</file>
		</directory>
		<directory name="cid">
			<file>type1cid.c</file>
		</directory>
		<directory name="gxvalid">
			<file>gxvalid.c</file>
		</directory>
		<directory name="gzip">
			<file>ftgzip.c</file>
		</directory>
		<directory name="lzw">
			<file>ftlzw.c</file>
		</directory>
		<directory name="otvalid">
			<file>otvalid.c</file>
		</directory>
		<directory name="pcf">
			<file>pcf.c</file>
		</directory>
		<directory name="pfr">
			<file>pfr.c</file>
		</directory>
		<directory name="psaux">
			<file>psaux.c</file>
		</directory>
		<directory name="pshinter">
			<file>pshinter.c</file>
		</directory>
		<directory name="psnames">
			<file>psnames.c</file>
		</directory>
		<directory name="raster">
			<file>raster.c</file>
		</directory>
		<directory name="sfnt">
			<file>sfnt.c</file>
		</directory>
		<directory name="smooth">
			<file>smooth.c</file>
		</directory>
		<directory name="truetype">
			<file>truetype.c</file>
		</directory>
		<directory name="type1">
			<file>type1.c</file>
		</directory>
		<directory name="type42">
			<file>type42.c</file>
		</directory>
		<directory name="winfonts">
			<file>winfnt.c</file>
		</directory>
	</directory>
	<file>rosglue.c</file>
	<file>freetype.rc</file>
</module>
