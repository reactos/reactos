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
        <file>DriverProc.c</file>
        <file>wodMessage.c</file>
        <file>widMessage.c</file>
        <file>modMessage.c</file>
        <file>midMessage.c</file>
        <file>mxdMessage.c</file>
        <file>auxMessage.c</file>
    </directory>
    <directory name="wave">
        <file>wavethread.c</file>
    </directory>
    <directory name="midi">
    </directory>
</module>
