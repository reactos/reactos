For reference see https://jira.reactos.org/browse/CORE-17005

This should be compiled using MSVC to create the image.dll file for testing.
Also, it creates an interactive showimg.exe program that can be used to verify this file.
After the creation of the image.dll file using MSVC it can be used to create user32_apitest:LoadImageGCC.
Simply copy the MSVC created 'image.dll' into the 'modules\rostests\apitests\user32' subdirectory to use it.
This file already exists there so that this step is not necessary, but only presents another option.

Unfortunately, this test will not work correctly using Windows 2003 Server SP2.
I have not looked into the details of why this is the case, but it works under ReactOS.
This is what is important for now in any case.

Doug Lyons
April 16, 2024
