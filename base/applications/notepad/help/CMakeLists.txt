
# NOTE: The help document filenames must be globally unique.
#       If you want to add image files, please add 'notepad-' prefix to the image filename.

# res/*
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/res/notepad.js DESTINATION reactos/Help/res FOR bootcd)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/res/notepad.css DESTINATION reactos/Help/res FOR bootcd)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/res/notepad-32x32.png DESTINATION reactos/Help/res FOR bootcd)

# *.txt
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/notepad_en-US.txt DESTINATION reactos/Help FOR bootcd) # English

# *.htm
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/notepad_en-US.htm DESTINATION reactos/Help FOR bootcd) # English
