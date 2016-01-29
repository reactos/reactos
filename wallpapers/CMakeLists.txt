
list(APPEND WALLPAPER_FILES
    Angelus_02_ROSWP_4-3_1440x1080.jpg
    Angelus_02_ROSWP_4-3_1024x768.jpg
    clouds.jpg
    lake.jpg
    ReactOS_CirclesBlue.bmp
    ReactOS_CirclesBlue.jpg
    ReactOS_CirclesGold.jpg
    ReactOS_Light_4-3.jpg
    ReactOS_Light_16-9.jpg
    ReadMe.txt)

foreach(item ${WALLPAPER_FILES})
    add_cd_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/${item}" DESTINATION reactos/Web/Wallpaper FOR all)
endforeach(item)
