# FontSub by Katayama Hirofumi MZ
# 
# To the extent possible under law, the person who associated CC0 with
# FontSub has waived all copyright and related or neighboring rights
# to FontSub.
# 
# You should have received a copy of the CC0 legalcode along with this
# work.  If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.

set_cpp(WITH_RUNTIME WITH_STL WITH_EXCEPTIONS)

add_executable(fontsubedit fontsub.cpp fontsub_res.rc)
set_module_type(fontsubedit win32gui UNICODE)
add_importlibs(fontsubedit advapi32 comctl32 comdlg32 shell32 gdi32 user32 msvcrt kernel32)
set_target_properties(fontsubedit PROPERTIES OUTPUT_NAME "fontsub")
add_cd_file(TARGET fontsubedit DESTINATION reactos/system32 FOR all)
