<?php
/*
  PROJECT:    ReactOS Website
  LICENSE:    GPL v2 or any later version
  FILE:       web/reactos.org/htdocs/getbuilds/index.php
  PURPOSE:    Easily download prebuilt ReactOS Revisions
  COPYRIGHT:  Copyright 2007-2008 Colin Finck <mail@colinfinck.de>
*/
	
	require_once("config.inc.php");
	require_once("languages.inc.php");
	
	// Get the domain for the cookie (with a leading dot if possible). Borrowed from "utils.php" of RosCMS.
	function cookie_domain()
	{
		if( isset($_SERVER['SERVER_NAME']) )
		{
			if( preg_match('/(\.[^.]+\.[^.]+$)/', $_SERVER['SERVER_NAME'], $matches) )
				return $matches[1];
			else
				return "";
		}
		else
			return "";
	}
	
	// Get the language and include it
	if( isset($_GET["lang"]) )
		$lang = $_GET["lang"];
	else if( isset($_COOKIE["roscms_usrset_lang"]) )
		$lang = $_COOKIE["roscms_usrset_lang"];
	
	// Check if the language is valid
	$lang_valid = false;
	
	foreach( $getbuilds_languages as $lang_key => $lang_name )
	{
		if( $lang == $lang_key )
		{
			$lang_valid = true;
			break;
		}
	}
	
	if( !$lang_valid )
		$lang = "en";
	
	setcookie( "roscms_usrset_lang", $lang, time() + 5 * 30 * 24 * 3600, "/", cookie_domain() );
	require_once("lang/$lang.inc.php");
	
	// Get the latest SVN revision
	$fp = fopen( $SVN_ACTIVITY_URL, "r" );
	
	do
	{
		$line = fread( $fp, 1024 );
		
		$firstpos = strpos( $line, "<id>" );
		
		if( $firstpos > 0 )
		{
			$lastpos = strpos( $line, "</id>" );
			$rev = substr( $line, $firstpos + 4, ($lastpos - $firstpos - 4) );
			break;
		}
	} while( $line != "" );
	
	fclose( $fp );
?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
	<meta http-equiv="content-type" content="text/html; charset=utf-8">
	<title><?php echo $getbuilds_langres["title"]; ?></title>
	<link rel="stylesheet" type="text/css" href="getbuilds.css">
	<!--[if lt IE 7]><link rel="stylesheet" type="text/css" href="ie-lt7-fixes.css"><![endif]-->
	<script type="text/javascript">
		document.write('<style type="text/css">');
		document.write('#js_stuff {display: block;}');
		document.write('<\/style>');
	</script>
	<script type="text/javascript">
		<?php require_once("getbuilds.js.php"); ?>
	</script>
</head>
<body onload="load();">

<?php
	readfile("http://www.reactos.org/$lang/subsys_extern_menu_top.html");
	readfile("http://www.reactos.org/$lang/subsys_extern_menu_left.html");
?>

<div class="navTitle"><?php echo $getbuilds_langres["language"]; ?></div>
	<ol>
		<li style="text-align: center;">
			<select size="1" onchange="window.location.href = '<?php echo "http://" . $_SERVER["SERVER_NAME"] . $_SERVER["PHP_SELF"]; ?>?lang=' + this.options[this.selectedIndex].value;">
				<?php
					foreach($getbuilds_languages as $lang_key => $lang_name)
						printf('<option value="%s"%s>%s</option>', $lang_key, ($lang_key == $lang ? ' selected' : ''), $lang_name);
				?>
			</select>
		</li>
	</ol>
</td>
<td id="content">

<h1><?php echo $getbuilds_langres["header"]; ?></h1>
<h2><?php echo $getbuilds_langres["title"]; ?></h2>

<p><?php echo $getbuilds_langres["intro"]; ?></p>

<div class="bubble_bg">
	<div class="rounded_ll">
	<div class="rounded_lr">
	<div class="rounded_ul">
	<div class="rounded_ur">
	
	<div class="bubble">
		<h3><?php echo $getbuilds_langres["overview"]; ?></h3>
		
		<?php echo $getbuilds_langres["latestrev"]; ?>: <strong><?php echo $rev; ?></strong>
		<ul class="web">
			<li><a href="http://svn.reactos.org/svn/reactos"><?php echo $getbuilds_langres["browsesvn"]; ?></a></li>
			<li><a href="http://cia.vc/stats/project/ReactOS"><?php echo $getbuilds_langres["cia"]; ?></a></li>
		</ul>
		
		<?php echo $getbuilds_langres["buildbot_status"]; ?>:
		<ul class="web">
			<li><a href="http://www.reactos.org:8010"><?php echo $getbuilds_langres["buildbot_web"]; ?></a></li>
			<li><a href="<?php echo $ISO_DOWNLOAD_URL; ?>"><?php echo $getbuilds_langres["browsebuilds"]; ?></a></li>
		</ul>
	</div>

	</div>
	</div>
	</div>
	</div>
</div>

<div class="bubble_bg">
	<div class="rounded_ll">
	<div class="rounded_lr">
	<div class="rounded_ul">
	<div class="rounded_ur">
	
	<div class="bubble">
		<h3><?php echo $getbuilds_langres["downloadrev"]; ?></h3>
		
		<noscript>
			<?php printf($getbuilds_langres["js_disclaimer"], $ISO_DOWNLOAD_URL); ?>
		</noscript>

		<div id="js_stuff">
			<table id="showrev" cellspacing="0" cellpadding="5">
				<tr>
					<td><?php echo addslashes($getbuilds_langres["showrevfiles"]); ?>: </td>
					<td>
						<span id="revcontrols">
							<img src="images/leftarrow.gif" alt="&lt;" title="<?php echo addslashes($getbuilds_langres["prevrev"]); ?>" onclick="prevRev();"> 
							<input type="text" id="revnum" value="<?php echo $rev; ?>" size="12" onkeyup="checkRevNum(this);"> 
							<img src="images/rightarrow.gif" alt="&gt;" title="<?php echo addslashes($getbuilds_langres["nextrev"]); ?>" onclick="nextRev();"><br>
						</span>
						
						<img src="images/info.gif" alt="INFO:"> <?php printf( addslashes($getbuilds_langres["rangeinfo"]), $rev, ($rev - 50), $rev ); ?>
					</td>
				</tr>
				<tr>
					<td><?php echo addslashes($getbuilds_langres["isotype"]); ?>: </td>
					<td>
						<input type="checkbox" id="bootcd-dbg" checked="checked"> Debug Boot CDs 
						<input type="checkbox" id="livecd-dbg" checked="checked"> Debug Live CDs 
						<input type="checkbox" id="bootcd-rel" checked="checked"> Release Boot CDs 
						<input type="checkbox" id="livecd-rel" checked="checked"> Release Live CDs
					</td>
				</tr>
			</table>
	
			<div id="controlbox">
				<input type="button" onclick="showRev();" value="<?php echo addslashes($getbuilds_langres["showrev"]); ?>" />
				
				<span id="ajaxloadinginfo">
					<img src="images/ajax_loading.gif" alt=""> <?php echo addslashes($getbuilds_langres["gettinglist"]); ?>...
				</span>
			</div>
	
			<div id="filetable">
				<table class="datatable" cellspacing="0" cellpadding="1">
					<thead>
						<tr class="head">
							<th class="fname"><?php echo addslashes($getbuilds_langres["filename"]); ?></th>
							<th class="fsize"><?php echo addslashes($getbuilds_langres["filesize"]); ?></th>
							<th class="fdate"><?php echo addslashes($getbuilds_langres["filedate"]); ?></th>
						</tr>
					</thead>
					<tbody>
						<tr class="odd">
							<td><?php echo addslashes($getbuilds_langres["pleasewait"]); ?>...</td>
							<td>&nbsp;</td>
							<td>&nbsp;</td>
						</tr>
					</tbody>
				</table>
			</div>
		</div>
	</div>
	
	</div>
	</div>
	</div>
	</div>
</div>

</td>
</tr>
</table>

</body>
</html>
