@echo off

set FXCTOOL=fxc
%FXCTOOL% /T vs_2_0 Source\ShaderEffectsVS.fx /E VS /FoBinary\ShaderEffectVS.vsbin
%FXCTOOL% /T vs_3_0 Source\ShaderEffectsVS.fx /E VS /FoBinary\ShaderEffectVS30.vsbin
%FXCTOOL% /T ps_2_0 Source\PassThroughShaderEffectPS.fx /E PS /FoBinary\PassThroughShaderEffectPS.psbin

@REM Blur Shaders
%FXCTOOL% /T ps_2_0 Source\HorizontalBlur.fx /E PS /FoBinary\HorizontalBlur.psbin
%FXCTOOL% /T ps_2_0 Source\VerticalBlur.fx /E PS /FoBinary\VerticalBlur.psbin
%FXCTOOL% /T ps_2_0 Source\HorizontalBlurMulti.fx /E PS /FoBinary\HorizontalBlurMulti.psbin
%FXCTOOL% /T ps_2_0 Source\VerticalBlurMulti.fx /E PS /FoBinary\VerticalBlurMulti.psbin

@REM Drop Shadow Shader
%FXCTOOL% /T ps_2_0 Source\DropShadow.fx /E PS /FoBinary\DropShadow.psbin