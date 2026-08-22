@echo off

::
::  Open all generated generated files for edit.
::

rem If this doesn't seem to edit all the generated files, you probably need
rem to add to tools\GeneratedFiles.txt

call %~dp0\sd_GeneratedFiles %SDXROOT%\wpf GeneratedResources.txt edit
