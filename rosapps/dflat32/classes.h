/* ----------- classes.h ------------ */
/*
 *         Class definition source file
 *         Make class changes to this source file
 *         Other source files will adapt
 *
 *         You must add entries to the DfColor tables in
 *         DFCONFIG.C for new classes.
 *
 *        Class Name  Base Class   Processor       Attribute    
 *       ------------  --------- ---------------  -----------
 */
DfClassDef(  DF_NORMAL,      -1,      DfNormalProc,      0 )
DfClassDef(  DF_APPLICATION, DF_NORMAL,  DfApplicationProc, DF_VISIBLE   |
                                                  DF_SAVESELF  |
                                                  DF_CONTROLBOX )
DfClassDef(  DF_TEXTBOX,     DF_NORMAL,  DfTextBoxProc,     0          )
DfClassDef(  DF_LISTBOX,     DF_TEXTBOX, DfListBoxProc,     0          )
DfClassDef(  DF_EDITBOX,     DF_TEXTBOX, DfEditBoxProc,     0          )
DfClassDef(  DF_MENUBAR,     DF_NORMAL,  DfMenuBarProc,     DF_NOCLIP     )
DfClassDef(  DF_POPDOWNMENU, DF_LISTBOX, DfPopDownProc,     DF_SAVESELF  |
                                                  DF_NOCLIP    |
                                                  DF_HASBORDER  )
#ifdef INCLUDE_PICTUREBOX
DfClassDef(  DF_PICTUREBOX,  DF_TEXTBOX, DfPictureProc,     0          )
#endif
DfClassDef(  DF_DIALOG,      DF_NORMAL,  DfDialogProc,      DF_SHADOW    |
                                                  DF_MOVEABLE  |
                                                  DF_CONTROLBOX|
                                                  DF_HASBORDER |
                                                  DF_NOCLIP     )
DfClassDef(  DF_BOX,         DF_NORMAL,  DfBoxProc,         DF_HASBORDER  )
DfClassDef(  DF_BUTTON,      DF_TEXTBOX, DfButtonProc,      DF_SHADOW     )
DfClassDef(  DF_COMBOBOX,    DF_EDITBOX, DfComboProc,       0          )
DfClassDef(  DF_TEXT,        DF_TEXTBOX, DfTextProc,        0          )
DfClassDef(  DF_RADIOBUTTON, DF_TEXTBOX, DfRadioButtonProc, 0          )
DfClassDef(  DF_CHECKBOX,    DF_TEXTBOX, DfCheckBoxProc,    0          )
DfClassDef(  DF_SPINBUTTON,  DF_LISTBOX, DfSpinButtonProc,  0          )
DfClassDef(  DF_ERRORBOX,    DF_DIALOG,  NULL,            DF_SHADOW    |
                                                  DF_HASBORDER  )
DfClassDef(  DF_MESSAGEBOX,  DF_DIALOG,  NULL,            DF_SHADOW    |
                                                  DF_HASBORDER  )
DfClassDef(  DF_HELPBOX,     DF_DIALOG,  DfHelpBoxProc,     DF_MOVEABLE  |
                                                  DF_SAVESELF  |
                                                  DF_HASBORDER |
                                                  DF_NOCLIP    |
                                                  DF_CONTROLBOX )
DfClassDef(  DF_STATUSBAR,   DF_TEXTBOX, DfStatusBarProc,   DF_NOCLIP     )

/*
 *  ========> Add new classes here <========
 */

/* ---------- pseudo classes to create enums, etc. ---------- */
DfClassDef(  DF_TITLEBAR,    -1,      NULL,            0          )
DfClassDef(  DF_DUMMY,       -1,      NULL,            DF_HASBORDER  )
