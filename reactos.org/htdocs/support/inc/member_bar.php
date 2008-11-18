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


	if($RSDB_intern_user_id != 0) {

?><div class="navTitle"><?php echo $RSDB_langres['Account']; ?></div>
	<ol>
		<li>
			&nbsp;Nick:&nbsp;<?php echo substr($RSDB_USER_name, 0, 9);
			
						
			
			 ?> <img src="images/info_15x15.gif" alt="<?php echo "Username: ".$RSDB_USER_name; ?>" width="15" height="15">
		</li>
		<li><a href="<?php echo $RSDB_intern_link_roscms_page; ?>logout"><?php echo $RSDB_langres['Logout']; ?></a></li>
	</ol>
</div>
<p></p><?php
	}
	else {
		?><form action="?page=login" method="post"><div class="navTitle">Login</div>   
		  <ol>
		<li><a href="<?php echo $RSDB_intern_link_roscms_page; ?>login"><?php echo $RSDB_langres['Global_Login_System']; ?></a></li>
		<li><a href="<?php echo $RSDB_intern_link_roscms_page; ?>register"><?php echo $RSDB_langres['Register_Account']; ?></a></li>
		  </ol></div></form>
		  <p></p><?php
	}
?>
