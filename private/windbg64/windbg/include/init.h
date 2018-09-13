/*++ BUILD Version: 0001    // Increment this if a change has global effects
---*/
/****************************************************************************

    PROTOTYPES DECLARATION FOR INIT MODULE

****************************************************************************/

// Inititialize a New QCQP Instance
BOOL
InitInstance(
    int argc,
    char * argv[],
    HINSTANCE hInstance,
    int nCmdShow
    );



//Iniitialize all possible files extension we handle
void
InitFileExtensions(
    void
    );

//Initialize default font
void
InitDefaultFont(
    void
    );

//Initialize default find and replace structure
void
InitDefaultFindReplace(
    void
    );

//Initialize workspace
void
InitWorkspace(
    void
    );

//Title bar data
void
PASCAL
InitTitleBar(
    void
    );

//Initialize all defaults
void
PASCAL
InitDefaults(
    void
    );
