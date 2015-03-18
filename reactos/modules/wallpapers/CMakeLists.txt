
list(APPEND WALLPAPER_FILES
    clouds.jpg
    grass.jpg
    lake.jpg
    ReactOS_CirclesBlue.jpg
    ReactOS_CirclesGold.jpg
    ReactOS_CirclesGold.bmp
    ReactOS_Light_4-3.jpg
    ReactOS_Light_16-9.jpg
    ReadMe.txt)

foreach(item ${WALLPAPER_FILES})
    add_cd_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/${item}" DESTINATION reactos/Web/Wallpaper FOR all)
endforeach(item)
