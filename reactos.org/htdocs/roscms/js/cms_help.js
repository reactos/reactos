



/**
 * @FILLME
 */
function highlightMenu( obj )
{
  // remove highlight from other entries
  if (obj != 'Info') {
    document.getElementById('lmInfo').style.backgroundColor = 'white';
    document.getElementById('lmInfo').style.fontWeight = 'normal';
    document.getElementById('pageInfo').style.display = 'none';
  }
  if (obj != 'EntryTable') {
    document.getElementById('lmEntryTable').style.backgroundColor = 'white';
    document.getElementById('lmEntryTable').style.fontWeight = 'normal';
    document.getElementById('pageEntryTable').style.display = 'none';
  }
  if (obj != 'WebEditor') {
    document.getElementById('lmWebEditor').style.backgroundColor = 'white';
    document.getElementById('lmWebEditor').style.fontWeight = 'normal';
    document.getElementById('pageWebEditor').style.display = 'none';
  }
  if (obj != 'Maintain') {
    document.getElementById('lmMaintain').style.backgroundColor = 'white';
    document.getElementById('lmMaintain').style.fontWeight = 'normal';
    document.getElementById('pageMaintain').style.display = 'none';
  }
  if (obj != 'Admin') {
    document.getElementById('lmAdmin').style.backgroundColor = 'white';
    document.getElementById('lmAdmin').style.fontWeight = 'normal';
    document.getElementById('pageAdmin').style.display = 'none';
  }
  if (obj != 'FAQ') {
    document.getElementById('lmFAQ').style.backgroundColor = 'white';
    document.getElementById('lmFAQ').style.fontWeight = 'normal';
    document.getElementById('pageFAQ').style.display = 'none';
  }
  if (obj != 'First') {
    document.getElementById('lmFirst').style.backgroundColor = 'white';
    document.getElementById('lmFirst').style.fontWeight = 'normal';
    document.getElementById('pageFirst').style.display = 'none';
  }

  // highlight
  document.getElementById('page'+obj).style.display = 'block';
  document.getElementById('lm'+obj).style.backgroundColor = '#C9DAF8';
  document.getElementById('lm'+obj).style.fontWeight = 'bold';

} // end of function highlightMenu



/**
 * @FILLME
 */
function loadInfo( )
{
  // highlight left menu entry
  highlightMenu('Info');
  return true;
} // end of function loadUserSearch



/**
 * @FILLME
 */
function loadEntryTable( )
{
  // highlight left menu entry
  highlightMenu('EntryTable');
  return true;
} // end of function loadUserSearch



/**
 * @FILLME
 */
function loadWebEditor( )
{
  // highlight left menu entry
  highlightMenu('WebEditor');
  return true;
} // end of function loadUserSearch



/**
 * @FILLME
 */
function loadMaintain( )
{
  // highlight left menu entry
  highlightMenu('Maintain');
  return true;
} // end of function loadUserSearch



/**
 * @FILLME
 */
function loadAdmin( )
{
  // highlight left menu entry
  highlightMenu('Admin');
  return true;
} // end of function loadUserSearch



/**
 * @FILLME
 */
function loadFAQ( )
{
  // highlight left menu entry
  highlightMenu('FAQ');
  return true;
} // end of function loadUserSearch



/**
 * @FILLME
 */
function loadFirstSteps( )
{
  // highlight left menu entry
  highlightMenu('First');
  return true;
} // end of function loadUserSearch


