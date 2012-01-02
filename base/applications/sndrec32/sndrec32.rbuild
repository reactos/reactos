<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="sndrec32" type="win32gui" installbase="system32" installname="sndrec32.exe">
	<include base="sndrec32">.</include>
	<library>winmm</library>
	<library>user32</library>
	<library>msacm32</library>
	<library>comctl32</library>
	<library>comdlg32</library>
	<library>gdi32</library>
	<file>audio_format.cpp</file>
	<file>audio_membuffer.cpp</file>
	<file>audio_producer.cpp</file>
	<file>audio_receiver.cpp</file>
	<file>audio_resampler_acm.cpp</file>
	<file>audio_wavein.cpp</file>
	<file>audio_waveout.cpp</file>
	<file>sndrec32.cpp</file>
	<file>rsrc.rc</file>
</module>
