<?php
/*
  PROJECT:    ReactOS Website
  LICENSE:    GNU GPLv2 or any later version as published by the Free Software Foundation
  PURPOSE:    Easily download prebuilt ReactOS Revisions
  COPYRIGHT:  Copyright 2007-2009 Colin Finck <mail@colinfinck.de>
*/

	define("ROOT_PATH", "../");
	
	require_once("config.inc.php");
	require_once("languages.inc.php");
	require_once(ROOT_PATH . "shared/subsys_layout.php");
	require_once(ROOT_PATH . "shared/svn.php");
	
	GetLanguage();
	require_once(ROOT_PATH . "shared/lang/$lang.inc.php");
	require_once("lang/$lang.inc.php");
	
	$rev = GetLatestRevision();
?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
	<meta http-equiv="content-type" content="text/html; charset=utf-8">
	<title><?php echo $getbuilds_langres["title"]; ?></title>
	<link rel="stylesheet" type="text/css" href="../shared/css/menu.css">
	<link rel="stylesheet" type="text/css" href="../shared/css/reactos.css">
	<link rel="stylesheet" type="text/css" href="getbuilds.css">
	<script type="text/javascript">
		document.write('<style type="text/css">');
		document.write('#js_stuff {display: block;}');
		document.write('<\/style>');
	</script>
	<script type="text/javascript" src="../shared/js/ajax.js"></script>
	<script type="text/javascript">
		<?php require_once("getbuilds.js.php"); ?>
	</script>
</head>
<body onload="Load();">

<?php
	BasicLayout($lang);
	LanguageBox($lang);
?>
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
		<h1><?php echo $getbuilds_langres["overview"]; ?></h1>
		
		<?php echo $getbuilds_langres["latestrev"]; ?>: <strong><?php echo $rev; ?></strong>
		<ul class="web">
			<li><a href="http://svn.reactos.org/svn/reactos"><?php echo $getbuilds_langres["browsesvn"]; ?></a></li>
			<li><a href="http://cia.vc/stats/project/ReactOS"><?php echo $getbuilds_langres["cia"]; ?></a></li>
		</ul>
		
		<?php echo $getbuilds_langres["buildbot_status"]; ?>:
		<ul class="web">
			<li><a href="http://build.reactos.org:8010"><?php echo $getbuilds_langres["buildbot_web"]; ?></a></li>
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
		<h1><?php echo $getbuilds_langres["downloadrev"]; ?></h1>
		
		<noscript>
			<?php printf($getbuilds_langres["js_disclaimer"], $ISO_DOWNLOAD_URL); ?>
		</noscript>

		<div id="js_stuff">
			<table id="showrev" cellspacing="0" cellpadding="5">
				<tr>
					<td><?php echo $getbuilds_langres["showrevfiles"]; ?>: </td>
					<td>
						<span id="revcontrols">
							<img src="images/leftarrow.gif" alt="&lt;" title="<?php echo $getbuilds_langres["prevrev"]; ?>" onclick="PrevRev();"> 
							<input type="text" id="revnum" value="<?php echo $rev; ?>" size="12" onkeyup="CheckRevNum(this);"> 
							<img src="images/rightarrow.gif" alt="&gt;" title="<?php echo $getbuilds_langres["nextrev"]; ?>" onclick="NextRev();"><br>
						</span>
						
						<img src="../shared/images/info.gif" alt=""> <?php printf($shared_langres["rangeinfo"], $rev, ($rev - 50), $rev); ?>
					</td>
				</tr>
				<tr>
					<td><?php echo $getbuilds_langres["isotype"]; ?>: </td>
					<td>
						<input type="checkbox" id="bootcd-dbg" checked="checked"> Debug Boot CDs 
						<input type="checkbox" id="livecd-dbg" checked="checked"> Debug Live CDs 
						<input type="checkbox" id="bootcd-rel" checked="checked"> Release Boot CDs 
						<input type="checkbox" id="livecd-rel" checked="checked"> Release Live CDs
					</td>
				</tr>
			</table>
	
			<div id="controlbox">
				<input type="button" onclick="ShowRev();" value="<?php echo $getbuilds_langres["showrev"]; ?>" />
				
				<span id="ajax_loading">
					<img src="../shared/images/ajax_loading.gif" alt=""> <?php echo $getbuilds_langres["gettinglist"]; ?>...
				</span>
			</div>
	
			<div id="filetable">
				<!-- Filled by the JavaScript -->
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
