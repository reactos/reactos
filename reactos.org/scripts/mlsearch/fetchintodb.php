<?
/*-------------------------------------------------------
*
* MBox Format Mailinglist Searcher
* You can index mbox files into a sql database and
* make a search over them afterwards.
*
* This work is licensed under the GPL.
*
* Copyright 2005 Michael Wirth
*
-------------------------------------------------------*/

set_time_limit(0);

require_once ('DB.php');
require_once ('connect.db.php');

$dsn = "mysql://$dbUser:$dbPass@$dbHost/$dbName";
$options = array(
    'debug'       => 2,
    'portability' => DB_PORTABILITY_ALL,
);

$db =& DB::connect($dsn, $options);
if (PEAR::isError($db)) {
    die($db->getMessage());
}
$db->setFetchMode(DB_FETCHMODE_ASSOC);




function changeMonth($mon) {
	switch($mon) {
		case "Jan":
			$month = 1;
			break;
		case "Feb":
			$month = 2;
			break;
		case "Mar":
			$month = 3;
			break;
		case "Apr":
			$month = 4;
			break;
		case "May":
			$month = 5;
			break;
		case "Jun":
			$month = 6;
			break;
		case "Jul":
			$month = 7;
			break;
		case "Aug":
			$month = 8;
			break;
		case "Sep":
			$month = 9;
			break;
		case "Oct":
			$month = 10;
			break;
		case "Nov":
			$month = 11;
			break;
		case "Dec":
			$month = 12;
			break;
	}
	return $month;
}

require_once "mbox.php";
require_once "mimeDecode.php";

function openMBOX($file, $id) {
global $db;

$mbox =& new Mail_Mbox();

$mbox->debug = true;

$mailbox = $mbox->open($file);

if ($mbox->size($mailbox) == 0) {
	print "No message found";
}



$params['include_bodies'] = true;
$params['decode_bodies']  = false;
$params['decode_headers'] = true;

for ($x = 0; $x < $mbox->size($mailbox); $x++)
{
	$content = "";
	$cont = "";

	$thisMessage = $mbox->get($mailbox,$x);
	$decode = new Mail_mimeDecode($thisMessage, "\r\n");
	$structure = $decode->decode($params);

	$date = explode(" ", $structure->headers['date']);
	$time = explode(":", $date[4]);
	
	//Timezone conversion
	if(substr($date[5], 0, 1) == "+") {
		$time[0] = intval($time[0]) - intval(substr($date[5], 1, 2));
	}
	elseif(substr($date[5], 0, 1) == "-") {
		$time[0] = intval($time[0]) + intval(substr($date[5], 1, 2));
	}
	
	$month = changeMonth($date[2]);
		
	$timestamp = gmmktime(intval($time[0]), intval($time[1]), intval($time[2]), intval($month), intval($date[1]), intval($date[3]));
	//echo date("r", $timestamp);
	//"j.m.y G:i"
	
	/*
	echo "<br>";
	echo $structure->headers['from'];
	echo "<br>";
	echo $structure->headers['subject'];
	echo "<br>";
	print_r($structure->body);
	*/
	//echo "<br>";
	
	$frommail = explode(" ", key($structure->headers));
	
	//$frommail = substr($structure->headers['from'], strpos($structure->headers['from'], "<") + 1, strpos($structure->headers['from'], ">", strpos($structure->headers['from'], "<") + 1) - strpos($structure->headers['from'], "<") - 1);
	
	if (strpos($structure->headers['from'], "<") > 0) $from = substr($structure->headers['from'], 0, strpos($structure->headers['from'], "<") - 1);
	else $from = $structure->headers['from'];
	$from = trim($from);
	$from = trim($from, "\"");
	
	if(!isset($structure->body)) {
		$i = 0;
		while($cont = $structure->parts[$i]) {
				if(substr($cont->headers['content-type'], 0, 10) == "text/plain" OR substr($cont->headers['content-type'], 0, 9) == "text/html") {
					$content = $cont->body;
					$i = -2;
				}
				$i++;
		}

		//$content = $content['body'];
		//print_r($structure->parts);
	}
	else $content = $structure->body;
	$content = trim($content);
	
	$sql = "INSERT INTO mailinglist_temp (id, mailinglist, frommail, f, sent, subject, content) VALUES ("
	. $db->quoteSmart($structure->headers['message-id'])
	. ", " . $id . ", '" . $frommail[1]
	. "', " . $db->quoteSmart($from)
	. ", " . $timestamp . ", "
	. $db->quoteSmart($structure->headers['subject'])
	. ", " . $db->quoteSmart($content) . ")";
	//echo $sql;
	
	if(DB::isError($db->query($sql))) {
		echo "Error in " . $structure->headers['message-id'] . "<br>";
	}
}
}

$db->query("CREATE TABLE mailinglist_temp LIKE mailinglist");

$dir = "/var/lib/mailman/archives/public/";

openMBOX($dir . "ros-dev.mbox/ros-dev.mbox", 1);
openMBOX($dir . "ros-general.mbox/ros-general.mbox", 0);
openMBOX($dir . "ros-announce.mbox/ros-announce.mbox", 2);
openMBOX($dir . "ros-web.mbox/ros-web.mbox", 5);
openMBOX($dir . "ros-foundation.mbox/ros-foundation.mbox", 6);
openMBOX($dir . "ros-translate.mbox/ros-translate.mbox", 7);
openMBOX($dir . "ros-svn.mbox/ros-svn.mbox", 3);
openMBOX($dir . "ros-svn-diffs.mbox/ros-svn-diffs.mbox", 4);
openMBOX($dir . "ros-kernel.mbox/ros-kernel.mbox", 11);
openMBOX($dir . "ros-cvs.mbox/ros-cvs.mbox", 12);
openMBOX($dir . "ros-diffs.mbox/ros-diffs.mbox", 10);
openMBOX($dir . "ros-errors.mbox/ros-errors.mbox", 13);
openMBOX($dir . "ros-cis.mbox/ros-cis.mbox", 14);
openMBOX($dir . "ros-bugs.mbox/ros-bugs.mbox", 15);

$db->query("INSERT INTO mailinglist SELECT * FROM mailinglist_temp WHERE mailinglist_temp.id NOT IN (SELECT id FROM mailinglist)");
$db->query("DROP TABLE mailinglist_temp");
?>
