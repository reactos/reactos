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



function noscript() {
	global $RSDB_intern_link_ajax;
	echo '<noscript>
	Your browser does not support JavaScript/JScript or (more likely) you have ECMAScript/JavaScript/JScript<br />
	disabled in your browser security option.<br /><br />
	To gain the best user interface experience please enable ECMAScript/JavaScript/JScript in your browser,<br />
	if possible.<br /><br />
	You will also be able to use the ReactOS Support Database without ECMAScript/JavaScript/JScript.<br /><br />
	<a href="'. $RSDB_intern_link_ajax .'false"><b>Please, click here to disable ECMAScript/JavaScript/JScript features</b></a> (DHTML & Ajax) on this page<br />
	and then you will be able to use all feature even without ECMAScript/JavaScript/JScript enabled!
	</noscript>';
}

?>