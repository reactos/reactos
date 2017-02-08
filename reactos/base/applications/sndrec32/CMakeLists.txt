
set_cpp(WITH_RUNTIME)

list(APPEND SOURCE
    audio_format.cpp
    audio_membuffer.cpp
    #audio_producer.cpp
    #audio_receiver.cpp
    audio_resampler_acm.cpp
    audio_wavein.cpp
    audio_waveout.cpp
    sndrec32.cpp
    stdafx.h)

file(GLOB sndrec32_rc_deps resources/*.*)
add_rc_deps(rsrc.rc ${sndrec32_rc_deps})
add_executable(sndrec32 ${SOURCE} rsrc.rc)
set_module_type(sndrec32 win32gui UNICODE)
add_importlibs(sndrec32 winmm user32 msacm32 comctl32 comdlg32 gdi32 shell32 msvcrt kernel32)
add_pch(sndrec32 stdafx.h SOURCE)
add_cd_file(TARGET sndrec32 DESTINATION reactos/system32 FOR all)
