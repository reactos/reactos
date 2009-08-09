<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="soundblaster" type="staticlibrary" allowwarnings="true">
    <define name="__NTDRIVER__"/>
    <define name="KERNEL"/>
    <include base="soundblaster">.</include>
    <include base="ReactOS">include/reactos/libs/sound</include>
    <file>dsp_io.c</file>
    <file>version.c</file>
    <file>speaker.c</file>
    <file>rate.c</file>
    <file>mixer.c</file>
</module>
