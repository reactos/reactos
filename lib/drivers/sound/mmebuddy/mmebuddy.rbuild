<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="mmebuddy" type="staticlibrary" allowwarnings="false" unicode="yes">
    <include base="ReactOS">include/reactos/libs/sound</include>
    <file>devices.c</file>
    <file>instances.c</file>
    <file>kernel.c</file>
    <file>nt4.c</file>
    <file>utility.c</file>
    <file>thread.c</file>
    <file>testing.c</file>
    <directory name="mme">
        <file>entry.c</file>
        <file>wod.c</file>
        <file>wid.c</file>
        <file>mod.c</file>
        <file>mid.c</file>
        <file>mxd.c</file>
        <file>aux.c</file>
    </directory>
    <directory name="wave">
        <file>wavethread.c</file>
    </directory>
    <directory name="midi">
    </directory>
</module>
