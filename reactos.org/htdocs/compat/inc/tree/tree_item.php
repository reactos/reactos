<?php
    /*
    RSDB - ReactOS Support Database
    Copyright (C) 2005-2006  Klemens Friedl <frik85@reactos.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
    */

/*
 *	ReactOS Support Database System - RSDB
 *	
 *	(c) by Klemens Friedl <frik85>
 *	
 *	2005 - 2006 
 */


	// To prevent hacking activity:
	if ( !defined('RSDB') )
	{
		die(" ");
	}



			switch (@$_GET['item2']) {
				case "details": // Details
				default:
					include('inc/comp/comp_item_details.php');
					break;
				case "screens": // Screenshots
					switch (@$_GET['addbox']) {
						case "add":
						case "submit":
							include('inc/comp/comp_item_screenshot_submit.php');
							break;
						case "":
						default:
							include('inc/comp/comp_item_screenshots.php');
							break;
					}
					break;
				case "tests": // Test Reports
					switch (@$_GET['addbox']) {
						case "":
						default:
							include('inc/comp/comp_item_tests.php');
							break;
						case "add":
						case "submit":
							include('inc/comp/comp_item_tests_submit.php');
							break;
					}
					break;
				case "forum": // Comments/Forum
					switch (@$_GET['addbox']) {
						case "":
						default:
							include('inc/comp/comp_item_forum.php');
							break;
						case "new":
							include('inc/comp/comp_item_forum.php');
							break;
					}
					break;
				case "bugs": // Known Bugs
					include('inc/comp/comp_item_bugs.php');
					break;
				case "tips": // Tips & Tricks
					include('inc/comp/comp_item_tips.php');
					break;
			}
	
?>
