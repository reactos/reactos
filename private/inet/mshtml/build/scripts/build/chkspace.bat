net use \\trigger\mailout /user:REDMOND\frm3bld _frm3bld
set !DSKSPACE=\f3qa\tools\x86\dskspace.exe
set MAIL=/t benchr /c t-jlong
:CHECK1
net use T: /delete
SET SIZE=600000000
SET SHARE=\\trigger\f3drop

  net use T: %SHARE% /user:REDMOND\frm3bld _frm3bld
  %!DSKSPACE% T:\ %SIZE%
  if not errorlevel 1 goto CHECK2
  echo Please remove unnecessary files from %SHARE% >T.T
  echo There should always be at least %SIZE% bytes. >>T.T
  echo. >>T.T
  DIR T:\ /ad >>T.T
  echo /S !! %SHARE%: DISK SPACE SHORTAGE! %MAIL% >>T.T
  copy t.t \\trigger\mailout\pending\chkspace.mail1

:CHECK2
net use T: /delete
SET SIZE=200000000
SET SHARE=\\trigger\TRIDENT

  net use T: %SHARE% /user:REDMOND\frm3bld _frm3bld
  %!DSKSPACE% T:\ %SIZE%
  if not errorlevel 1 goto CHECK3
  echo Please remove unnecessary files from %SHARE% >T.T
  echo There should always be at least %SIZE% bytes. >>T.T
  echo. >>T.T
  DIR T:\ /ad >>T.T
  echo /S !! %SHARE%: DISK SPACE SHORTAGE! %MAIL% >>T.T
  copy t.t \\trigger\mailout\pending\chkspace.mail2

:CHECK3
net use T: /delete
SET SIZE=20000000
SET SHARE=\\axtest\www

  net use T: %SHARE% /user:REDMOND\frm3bld _frm3bld
  %!DSKSPACE% T:\ %SIZE%
  if not errorlevel 1 goto CHECK4
  echo Please remove unnecessary files from %SHARE% >T.T
  echo There should always be at least %SIZE% bytes. >>T.T
  echo. >>T.T
  DIR T:\ /ad >>T.T
  echo /S !! %SHARE%: DISK SPACE SHORTAGE! %MAIL% >>T.T
  copy t.t \\trigger\mailout\pending\chkspace.mail3

:CHECK4
net use T: /delete
SET SIZE=20000000
SET SHARE=\\f3qa\tp3

  net use T: %SHARE% /user:REDMOND\frm3bld _frm3bld
  %!DSKSPACE% T:\ %SIZE%
  if not errorlevel 1 goto CHECK5
  echo Please remove unnecessary files from %SHARE% >T.T
  echo There should always be at least %SIZE% bytes. >>T.T
  echo. >>T.T
  DIR T:\ /ad >>T.T
  echo /S !! %SHARE%: DISK SPACE SHORTAGE! %MAIL% >>T.T
  copy t.t \\trigger\mailout\pending\chkspace.mail4

:CHECK5
net use T: /delete
SET SIZE=20000000
SET SHARE=\\forms3\www

  net use T: %SHARE% /user:REDMOND\frm3bld _frm3bld
  %!DSKSPACE% T:\ %SIZE%
  if not errorlevel 1 goto CHECK6
  echo Please remove unnecessary files from %SHARE% >T.T
  echo There should always be at least %SIZE% bytes. >>T.T
  echo. >>T.T
  DIR T:\ /ad >>T.T
  echo /S !! %SHARE%: DISK SPACE SHORTAGE! %MAIL% >>T.T
  copy t.t \\trigger\mailout\pending\chkspace.mail5

:CHECK6
net use T: /delete
net use \\trigger\mailout /delete