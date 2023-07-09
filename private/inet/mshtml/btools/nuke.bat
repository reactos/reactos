@echo off
cd pdlparse
delnode /q obj objd objp objm
cd ..\ascparse
delnode /q obj objd objp objm
cd ..\nfparse
delnode /q obj objd objp objm
cd ..
del /q/s build*.log 1>nul: 2>nul:
