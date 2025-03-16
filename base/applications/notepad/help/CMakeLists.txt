# res
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/res/notepad.png DESTINATION reactos/Help/res FOR bootcd)

# *.txt
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/notepad_en.txt DESTINATION reactos/Help FOR bootcd) # English

# *.html
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/notepad_en.html DESTINATION reactos/Help FOR bootcd) # English
