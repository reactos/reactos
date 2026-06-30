set clrpath=%MANAGED_TOOLS_ROOT%\%MANAGED_REFS_VERSION%

:: Enable generation outside of the build environment.
if "%clr_ref_path%"=="" (
    set clr_ref_path=%MANAGED_TOOLS_ROOT%\%MANAGED_REFS_VERSION%
)
