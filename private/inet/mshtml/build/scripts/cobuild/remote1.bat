rem USAGE:
rem remote1 username
rem performs project sync
rem
rem
rem
title SSyncing for %1
echo. >sync.log
echo. ********************** PROJECT SSYNC ********************** >>sync.log
ssync -! -faq -l sync.log
echo. *********************************************************** >>sync.log
copy sync.log %_COMMUNICATE%\%1.log