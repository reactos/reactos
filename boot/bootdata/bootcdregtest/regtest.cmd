@echo off
set WINETEST_DEBUG=0
dbgprint --process "ipconfig"
start rosautotest /r /s
