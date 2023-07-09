rem USAGE:
rem remote2 username buildtype path
rem Builds project
rem
rem
rem

title Building %2 %3 for %1
pushd \forms96\build\%3
echo. >build.log
echo. ************** Building %2 %3 ********** >>build.log
setlocal
call make.bat %2 2>&1 | tee build.log
endlocal
echo. *********************************************************** >>build.log
copy build.log %_COMMUNICATE%\%1.log
popd
now %1 Built %2 %3 >>c:\rbuild.log