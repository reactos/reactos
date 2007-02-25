<module name="vfatxlib" type="staticlibrary">
	<include base="vfatxlib">.</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="__USE_W32API" />
	<file>fatx.c</file>
	<file>vfatxlib.c</file>
</module>
