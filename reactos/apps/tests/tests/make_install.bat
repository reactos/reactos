echo off
REM
REM - This is kinda dirty, I might fix it up later. - SE
REM

REM - Make System

cd GetSystemInfo
make
cd ..
cd guitest
make
cd ..
cd hello
make
cd ..
cd hello2
make
cd ..
cd Mutex
make
cd ..
cd new
make
cd ..
cd Parent_Child
make
cd ..
cd rolex
make
cd ..
cd volinfo
make
cd ..

REM - installs

mkdir C:\tests
copy GetSystemInfo\GetSystemInfo.exe  C:\tests
copy guitest\guitest.exe C:\tests
copy hello\hello.exe C:\tests
copy hello2\hello2.exe C:\tests
copy Mutex\fivemutex.exe C:\tests
copy Mutex\rapidmutex.exe C:\tests
copy Parent_Child\Parent_Child.exe C:\tests
copy rolex\rolex.exe C:\tests
copy volinfo\volinfo.exe C:\tests

