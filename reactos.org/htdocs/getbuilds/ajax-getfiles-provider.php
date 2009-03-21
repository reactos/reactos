<?php
/*
  PROJECT:    ReactOS Website
  LICENSE:    GNU GPLv2 or any later version as published by the Free Software Foundation
  PURPOSE:    Easily download prebuilt ReactOS Revisions
  COPYRIGHT:  Copyright 2007-2008 Colin Finck <mail@colinfinck.de>
*/
   
	// This "ajax-getfiles.php" script has to be uploaded to the server, which contains the ISO files.
	// Therefore it has an own configuration and doesn't use "config.inc.php".

	// Configuration
	$ROOT_DIR = "../";
	$MAX_FILES_PER_PAGE = 100;			// The same value has to be set in "config.inc.php"
	$REV_RANGE_LIMIT = 3000;

	// Functions
	function fsize_str($size)
	{
		if($size > 1000000)
		{
			$size = $size / 1048576;
			$unit = " MB";
		}
		else if($size > 1000)
		{
			$size = $size / 1024;
			$unit = " KB";
		}
		else
		{
			$unit = " Bytes";
		}
		
		return number_format($size, 2, ".", ",") . $unit;
	}
	
	
	// Entry point
	header("Content-type: text/xml");
	
	if(!isset($_GET["startrev"]))
		die("<error><message>Necessary information not specified!</message></error>");
	
	if($_GET["endrev"] - $_GET["startrev"] > $REV_RANGE_LIMIT)
		die("<error><message>LIMIT</message><limit>$REV_RANGE_LIMIT</limit></error>");
	
	if($_GET["filelist"])
		$get_filelist = true;
	
	$directories = array("bootcd", "livecd");
	$file_patterns = array();
	
	if($_GET["bootcd-dbg"])
		$file_patterns[] = "#bootcd-[0-9]+-dbg#";
	if($_GET["livecd-dbg"])
		$file_patterns[] = "#livecd-[0-9]+-dbg#";
	if($_GET["bootcd-rel"])
		$file_patterns[] = "#bootcd-[0-9]+-rel#";
	if($_GET["livecd-rel"])
		$file_patterns[] = "#livecd-[0-9]+-rel#";
	
	$exitloop = false;
	$filecount = 0;
	$firstrev = 0;
	$lastrev = 0;
	$morefiles = 0;
	
	foreach($directories as $d)
	{
		$dir = opendir($ROOT_DIR . $d) or die("<error><message>opendir failed!</message></error>");
	
		while($fname = readdir($dir))
			if(preg_match("#-([0-9]+)-#", $fname, $matches))
				$fnames[ $matches[1] ][] = $fname;
		
		closedir($dir);
	}
	
	echo "<fileinformation>";
	
	for($i = $_GET["startrev"]; $i <= $_GET["endrev"]; $i++)
	{
		if(isset($fnames[$i]))
		{
			sort($fnames[$i]);
			
			foreach($fnames[$i] as $fname)
			{
				// Is it an allowed CD Image type?
				foreach($file_patterns as $p)
				{
					if(preg_match($p, $fname))
					{
						// This is a file we are looking for
						if($get_filelist)
						{
							$dir = substr($fname, 0, 6);
							
							echo "<file>";
							printf("<name>%s</name>", $fname);
							printf("<size>%s</size>", fsize_str(filesize("$ROOT_DIR/$dir/$fname")));
							printf("<date>%s</date>", date("Y-m-d H:i", filemtime("$ROOT_DIR/$dir/$fname")));
							echo "</file>";
						}
					
						if($i < $firstrev || $firstrev == 0)
							$firstrev = $i;
				
						if($i > $lastrev)
							$lastrev = $i;
						
						$filecount++;
						break;
					}
				}
				
				if($filecount == $MAX_FILES_PER_PAGE)
				{
					$morefiles = 1;
					$exitloop = true;
					break;
				}
			}
		}
		
		if($exitloop)
			break;
	}
	
	printf("<filecount>%d</filecount>", $filecount);
	printf("<firstrev>%d</firstrev>", $firstrev);
	printf("<lastrev>%d</lastrev>", $lastrev);
	printf("<morefiles>%d</morefiles>", $morefiles);
	
	echo "</fileinformation>";
?>
