echo off
REM
REM - This is kinda dirty, I might fix it up later. - SE
REM

REM - Make System

cd GetSysMetrics
make
cd ..
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

mkdir C:\reactos\bin\tests
copy GetSystemInfo\GetSystemInfo.exe  C:\reactos\bin\tests
copy guitest\guitest.exe C:\reactos\bin\tests
copy hello\hello.exe C:\reactos\bin\tests
copy hello2\hello2.exe C:\reactos\bin\tests
copy Mutex\fivemutex.exe C:\reactos\bin\tests
copy Mutex\rapidmutex.exe C:\reactos\bin\tests
copy Parent_Child\Parent_Child.exe C:\reactos\bin\tests
copy rolex\rolex.exe C:\reactos\bin\tests
copy volinfo\volinfo.exe C:\reactos\bin\tests

