
set_unicode()
set_cpp()
set_rc_compiler()

add_executable(sndrec32
    audio_format.cpp
    audio_membuffer.cpp
    audio_producer.cpp
    audio_receiver.cpp
    audio_resampler_acm.cpp
    audio_wavein.cpp
    audio_waveout.cpp
    sndrec32.cpp
    rsrc.rc)

target_link_libraries(sndrec32 stlport)
set_module_type(sndrec32 win32gui)
add_importlibs(sndrec32 winmm user32 msacm32 comctl32 comdlg32 gdi32 msvcrt kernel32)
add_cd_file(TARGET sndrec32 DESTINATION reactos/system32)
