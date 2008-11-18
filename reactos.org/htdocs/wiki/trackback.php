<?php
/**
 * Provide functions to handle article trackbacks.
 * @file
 * @ingroup SpecialPage
 */
require_once( './includes/WebStart.php' );
require_once( './includes/DatabaseFunctions.php' );

/**
 *
 */
function XMLsuccess() {
	header("Content-Type: application/xml; charset=utf-8");
	echo "<?xml version=\"1.0\" encoding=\"utf-8\"?>
<response>
<error>0</error>
</response>
	";
	exit;
}

function XMLerror($err = "Invalid request.") {
	header("HTTP/1.0 400 Bad Request");
	header("Content-Type: application/xml; charset=utf-8");
	echo "<?xml version=\"1.0\" encoding=\"utf-8\"?>
<response>
<error>1</error>
<message>Invalid request: $err</message>
</response>
";
		exit;
}

if (!$wgUseTrackbacks)
	XMLerror("Trackbacks are disabled.");

if (   !isset($_POST['url'])
    || !isset($_REQUEST['article']))
	XMLerror("Required field not specified");

$dbw = wfGetDB(DB_MASTER);

$tbtitle = strval( @$_POST['title'] );
$tbex = strval( @$_POST['excerpt'] );
$tburl = strval( $_POST['url'] );
$tbname = strval( @$_POST['blog_name'] );
$tbarticle = strval( $_REQUEST['article'] );

$title = Title::newFromText($tbarticle);
if (!isset($title) || !$title->exists())
	XMLerror("Specified article does not exist.");

$dbw->insert('trackbacks', array(
	'tb_page'	=> $title->getArticleID(),
	'tb_title'	=> $tbtitle,
	'tb_url'	=> $tburl,
	'tb_ex'		=> $tbex,
	'tb_name'	=> $tbname
));
$dbw->commit();

XMLsuccess();

?>
