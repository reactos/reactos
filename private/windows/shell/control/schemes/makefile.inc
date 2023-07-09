..\..\..\windows\inc\winuserp.h : ..\..\..\windows\inc\winuserp.x
    wcshdr < ..\..\..\windows\inc\winuserp.x > ..\..\..\windows\inc\winuserp.h


..\..\..\windows\inc\winuser.x ..\..\..\windows\inc\winuserp.x: ..\..\..\windows\inc\winuser.w
    hsplit -o ..\..\..\windows\inc\winuser.x ..\..\..\windows\inc\winuserp.x ..\..\..\windows\inc\winuser.w
