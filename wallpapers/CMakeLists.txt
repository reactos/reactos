
list(APPEND WALLPAPER_FILES
    Angelus_DeepSea_16-9_1920x1080.jpg
    Angelus_DeepSea_4-3_1440x1080.jpg
    Angelus_ReactOS_Blue_16-9.jpg
    Angelus_ReactOS_Blue_4-3.jpg
    Angelus_ReactOS_Dark_16-9.jpg
    Angelus_ReactOS_Dark_4-3.jpg
    Angelus_ReactOS_Light_16-9.jpg
    Angelus_ReactOS_Light_4-3.jpg
    Angelus_ROSWP_16-9_1920x1080.jpg
    Angelus_ROSWP_4-3_1024x768.jpg
    Angelus_ROSWP_4-3_1440x1080.jpg
    Angelus_Sky4_16x9.jpg
    Angelus_Sky4_4x3.jpg
    Angelus_Sky5_16x9.jpg
    Angelus_Sky5_4x3.jpg
    Angelus_Sky7_16x9.jpg
    Angelus_Sky7_4x3.jpg
    lake.jpg
    ReactOS_CirclesBlue_1.jpg
    ReactOS_CirclesBlue_2.jpg
    ReactOS_CirclesGold_1.jpg
    ReactOS_CirclesGold_2.jpg
    ReactOS_FOSDEM_1.bmp
    ReactOS_FOSDEM_1.jpg
    ReactOS_FOSDEM_2.jpg
    ReadMe.txt)

foreach(item ${WALLPAPER_FILES})
    add_cd_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/${item}" DESTINATION reactos/Web/Wallpaper FOR all)
endforeach(item)
