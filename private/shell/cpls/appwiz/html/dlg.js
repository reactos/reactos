// Dlg class.  
//
// Provides some helper methods to resize a dialog window.

function Dlg()
{
    this._cxPerEm = 1;
    this._cyPerEm = 1;

    this._dxFrame = 0;
    this._dyFrame = 0;
}


/*-------------------------------------------------------------------------
Purpose: Calculates pixels-per-em metrics, based upon the given dialog size string
         ("dialogWidth:xxx; dialogHeight:yyy").

         The body of the dialog requires a DIV with width and height set to 100%.
         This is needed so we can account for the frame of the dialog when we
         resize (client vs. window rect).
         
*/
function Dlg_CalcMetrics(szDlgSize, idDivDlg)
{
    // There's a race condition where offsetHeight can change in the middle of this
    // function if the content is greater than the window.  So take a snapshot of it
    // now.
    var cxDiv = idDivDlg.offsetWidth;
    var cyDiv = idDivDlg.offsetHeight;
    
    var ichWidth = szDlgSize.indexOf("dialogWidth:") + 12;   // 12 = cch of "dialogWidth:"
    var ichHeight = szDlgSize.indexOf("dialogHeight:") + 13; // 13 = cch of "dialogHeight:"
    var cxDlgEm = parseInt(szDlgSize.substring(ichWidth));
    var cyDlgEm = parseInt(szDlgSize.substring(ichHeight));

    /*
    alert('dialogWidth=' + window.dialogWidth + '; offsetWidth=' + cxDiv +
          '; clientWidth=' + idDivDlg.clientWidth + '\n' +
          'dialogHeight=' + window.dialogHeight + '; offsetHeight=' + cyDiv +
          '; clientHeight=' + idDivDlg.clientHeight + '\n' +
          'cxDlgEm=' + cxDlgEm + '; cyDlgEm=' + cyDlgEm);
    */
    
    // dialogWidth is in pixels when read, but is in ems when written.
    // The inconsistency amazes me...but I'll make use of that fact!
    
    var cxWindow = parseInt(window.dialogWidth);        // we expect this to be in pixels
    var cyWindow = parseInt(window.dialogHeight);       // we expect this to be in pixels
    
    // We need to set the window size of the dialog, which is bigger than the client
    // size.  So calculate the difference so we can adjust the rectangle appropriately.
    
    this._dxFrame = cxWindow - cxDiv;
    this._dyFrame = cyWindow - cyDiv;
    
    this._cxPerEm = cxWindow / cxDlgEm;
    this._cyPerEm = cyWindow / cyDlgEm;
}


function Dlg_CxToEms(cx)
{
    return cx / this._cxPerEm;
}


function Dlg_CyToEms(cy)
{
    return cy / this._cyPerEm;
}


/*-------------------------------------------------------------------------
Purpose: Resize the dialog to the given width and height (in pixels).
*/
function Dlg_Resize(cx, cy)
{
    var cxNew = this.CxToEms(cx + this._dxFrame);
    var cyNew = this.CyToEms(cy + this._dyFrame);
    
    // Set the dialog size to entirely accomodate the contents of the dialog
    /*
    alert('cx=' + cx + '; cxNew=' + cxNew + '; this._dxFrame=' + this._dxFrame + '\n' +
          'cy=' + cy + '; cyNew=' + cyNew + '; this._dyFrame=' + this._dyFrame);
    */
    
    // We cannot simply use the ResizeTo or ResizeBy methods because they don't
    // work for dialog windows.
    
    window.dialogWidth = cxNew;
    window.dialogHeight = cyNew;
}



/*-------------------------------------------------------------------------
Purpose: Initialize the Dlg class
*/
function InitDlgClass()
{
    // Create and discard an initial Dlg object for prototypes
    new Dlg();

    // Dlg Methods
    Dlg.prototype.CalcMetrics = Dlg_CalcMetrics;
    Dlg.prototype.Resize = Dlg_Resize;
    Dlg.prototype.CxToEms = Dlg_CxToEms;
    Dlg.prototype.CyToEms = Dlg_CyToEms;
}



