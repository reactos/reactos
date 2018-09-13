//***   uemedat.h -- generator file for UEME_*
// NOTES
//  this file is included in numerous contexts w/ TABDAT #defined
//  to give the desired behavior.   see the individ files for details.
//
//  BUGBUG many of these are NYI.
//
//  e<n>    encode up to step <= n
//  f       fire event
//  l       log event
//  @       escape to custom code
//  !       ASSERT
//  x       NYI
//

// UI (menu, shortcut, etc.)
TABDAT(UEME_UIMENU     , "e2fl@" , 0, 0, 0, 0)
TABDAT(UEME_UIHOTKEY   , "e1fl"  , 0, 0, 0, 0)
TABDAT(UEME_UISCUT     , "e1fl"  , 0, 0, 0, 0)
TABDAT(UEME_UIQCUT     , "e1fl"  , 0, 0, 0, 0)
TABDAT(UEME_UITOOLBAR  , "e2fl"  , 0, 0, 0, 0)
#if 0 // 980825 uncomment in uemevt.h, uemedat.h if build breaks (tmp hack)
TABDAT(UEME_UIASSOC    , "e1fl"  , 0, 0, 0, 0)
#endif

// run (spawn, invoke, etc.)
TABDAT(UEME_RUNWMCMD   , "e2fl"  , 0, 0, 0, 0)
TABDAT(_UEME_RUNPIDL1  , "e2fl"  , 0, 0, 0, 0)  // obsolete, remove
TABDAT(UEME_RUNPIDL    , "e2fl"  , 0, 0, 0, 0)
TABDAT(UEME_RUNINVOKE  , "xe1fl" , 0, 0, 0, 0)
TABDAT(UEME_RUNOLECMD  , "xe1fl" , 0, 0, 0, 0)
TABDAT(UEME_RUNPATHA   , "e2fl"  , 0, 0, 0, 0)
TABDAT(UEME_RUNPATHW   , "e2fl"  , 0, 0, 0, 0)
TABDAT(UEME_RUNCPLA    , "e2fl"  , 0, 0, 0, 0)
TABDAT(UEME_RUNCPLW    , "e2fl"  , 0, 0, 0, 0)

// exit status
TABDAT(UEME_DONECANCEL , "e1fl"  , 0, 0, 0, 0)
TABDAT(UEME_DONEOK     , "e1fl"  , 0, 0, 0, 0)
TABDAT(UEME_DONEFAIL   , "e1fl"  , 0, 0, 0, 0)

// error
// NOTES
//  for now lParam=szMsg, that's just temporary but not sure what we need yet
TABDAT(UEME_ERRORA     , "e1fl"  , 0, 0, 0, 0)
TABDAT(UEME_ERRORW     , "e1fl"  , 0, 0, 0, 0)

#ifdef UNICODE
TABDAT(UEME_ERROR      , "e1fl"  , 0, 0, 0, 0)
#else
TABDAT(UEME_ERROR      , "e1fl"  , 0, 0, 0, 0)
#endif

// control
TABDAT(UEME_CTLSESSION , "@"     , 0, 0, 0, 0)

// debug
TABDAT(UEME_DBTRACEA   , "@"     , 0, 0, 0, 0)
TABDAT(UEME_DBTRACEW   , "@"     , 0, 0, 0, 0)

#ifdef UNICODE
TABDAT(UEME_DBTRACE    , "@"     , 0, 0, 0, 0)
#else
TABDAT(UEME_DBTRACE    , "@"     , 0, 0, 0, 0)
#endif
TABDAT(UEME_DBSLEEP    , "@"     , 0, 0, 0, 0)

// Instrumented Browser
TABDAT(UEME_INSTRBROWSER, "e2fl" , 0, 0, 0, 0)

// all events below here (msg < UEME_USER) are reserved
// private messages start here (at UEME_USER + 0)
// BUGBUG NYI we don't support private messages for now
TABDAT(UEME_USER       , "x"     , 0, 0, 0, 0)

