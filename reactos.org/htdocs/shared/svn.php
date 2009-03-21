<?php
/*
  PROJECT:    ReactOS Shared Website Components
  LICENSE:    GNU GPLv2 or any later version as published by the Free Software Foundation
  PURPOSE:    SVN functions for subsystems
  COPYRIGHT:  Copyright 2008-2009 Colin Finck <colin@reactos.org>
*/

	function GetLatestRevision()
	{
		$fp = fopen("http://svn.reactos.org/svnact/svn_activity.xml", "r");
		
		do
		{
			$line = fread($fp, 1024);
			
			$firstpos = strpos($line, "<id>");
			
			if($firstpos > 0)
			{
				$lastpos = strpos($line, "</id>");
				return substr($line, $firstpos + 4, ($lastpos - $firstpos - 4));
			}
		} while($line);
		
		fclose($fp);
	}
?>
