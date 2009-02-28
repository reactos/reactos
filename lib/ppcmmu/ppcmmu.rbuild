<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../tools/rbuild/project.dtd">
<module name="ppcmmu" type="staticlibrary">
        <include base="ppcmmu">.</include>
        <define name="__NO_CTYPE_INLINES" />
        <if property="ARCH" value="powerpc">
                <file>mmuutil.c</file>
        </if>
	<file>dummy.c</file>
</module>
