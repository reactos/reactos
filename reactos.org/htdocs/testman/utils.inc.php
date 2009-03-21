<?php
/*
  PROJECT:    ReactOS Web Test Manager
  LICENSE:    GNU GPLv2 or any later version as published by the Free Software Foundation
  PURPOSE:    Utility functions shared among several PHP files
  COPYRIGHT:  Copyright 2008-2009 Colin Finck <colin@reactos.org>
*/
		
	function GetPlatformString($platform)
	{
		// First get the main operating system
		if(substr($platform, 0, 7) == "reactos")
		{
			$str = "ReactOS";
			$arch = (int)substr($platform, 8);
		}
		else
		{
			sscanf($platform, "%u.%u.%u.%u.%u.%c.%u", $major, $minor, $build, $sp_major, $sp_minor, $type, $arch);
			
			switch($major)
			{
				case 5:
					switch($minor)
					{
						case 0:
							$str = "Windows 2000";
							$final_builds = array(2195);
							
							if($type == "s")
								$str .= " Server";
							
							break;
						
						case 1:
							$str = "Windows XP";
							$final_builds = array(2600);
							break;
						
						case 2:
							$final_builds = array(3790);
							
							if($type == "s")
								$str = "Windows Server 2003";
							else
								$str = "Windows XP";
							
							break;
						
						default:
							return $platform;
					}
					
					break;
				
				case 6:
					switch($minor)
					{
						case 0:
							$final_builds = array(6000, 6001);
							
							if($type = "s")
								$str = "Windows Server 2008";
							else
								$str = "Windows Vista";
							
							break;
						
						case 1:
							$final_builds = array();
							
							$str = "Windows 7";
							break;

						default:
							return $platform;
					}
					
					break;
				
				default:
					return $platform;
			}

			// Check if our current build is a final build. If it's not, also show the build number.
			$found = false;
			
			foreach($final_builds as $b)
			{
				if($b == $build)
				{
					$found = true;
					break;
				}
			}
			
			if(!$found)
				$str .= " - Build " . $build;
	
			// Add the service pack information if we have any
			if($sp_major)
			{
				$str .= " SP" . $sp_major;
				
				if($sp_minor)
					$str .= "." . $sp_minor;
			}
		}
		
		// Now get the processor architecture
		// The values for $arch are Windows PROCESSOR_ARCHITECTURE_* constants
		$str .= " - ";
		
		switch($arch)
		{
			case 0: $str .= "i386"; break;
			case 9: $str .= "AMD64"; break;
			default: return $platform;
		}
		
		return $str;
	}
	
	function GetDateString($timestamp)
	{
		return date("Y-m-d H:i", $timestamp);
	}
	
	function GetTotalTestsString($count)
	{
		// The number of total tests being -1 indicates that the test crashed while running
		if($count == -1)
			return "CRASH";
		
		return $count;
	}
?>
