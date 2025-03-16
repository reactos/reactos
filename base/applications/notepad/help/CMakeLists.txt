# res
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/res/notepad.png DESTINATION reactos/Help/res FOR bootcd)

# *.txt
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/notepad_en-US.txt DESTINATION reactos/Help FOR bootcd) # English

# *.html
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/notepad_en-US.html DESTINATION reactos/Help FOR bootcd) # English
