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


  $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp WHERE comp_visible = '1' AND comp_id = :comp_id ORDER BY comp_name ASC");
  $stmt->bindParam('comp_id',$RSDB_SET_item,PDO::PARAM_STR);
  $stmt->execute();
	
	$result_page = $stmt->fetch(PDO::FETCH_ASSOC);
	
	
	echo "<h2>".$result_page['comp_name'] ." [". "ReactOS ".show_osversion($result_page['comp_osversion']) ."]</h2>"; 
	
	include("inc/comp/comp_item_menubar.php");
	
?>
 <p>Bugzilla integration is still on the TODO list ... </p>
 <p><br />
 Sample table: 
  </p>
 <table border="0" cellpadding="3" cellspacing="1" width="100%">
            <tr bgcolor="#5984C3" class="color4">
              <td width="80" align="center"><strong><font color="#FFFFFF" size="2"><span class="Stil7">Bug #</span></font></strong></td>
              <td><strong><font color="#FFFFFF" size="2"><span class="Stil7">Description</span></font></strong></td>
              <td width="80" align="center"><strong><font color="#FFFFFF" size="2"><span class="Stil7">Status</span></font></strong></td>
              <td width="80" align="center"><strong><font color="#FFFFFF" size="2"><span class="Stil7">Resolution</span></font></strong></td>
              <td width="120" align="center" bgcolor="#5984C3"><strong><font color="#FFFFFF" size="2"><span class="Stil7">Other Apps affected</span></font></strong></td>
  </tr>
            <tr bgcolor="#E2E2E2" class="color0">
              <td align="center"><font size="2"><a href="http://www.reactos.org/bugzilla/show_bug.cgi?id=1284" class="Stil3">1284</a></font></td>
              <td><font size="2"><span class="Stil3">Weird icons in title bar while windowed</span></font></td>
              <td align="center"><font size="2"><span class="Stil3">CLOSED</span></font></td>
              <td align="center"><font size="2"><span class="Stil3">WORKSFORME</span></font></td>
              <td align="center"><font size="2"><a href="<?php echo $RSDB_intern_link_db_view_comp; ?>&amp;bug=1284" class="Stil3">View</a></font></td>
  </tr>
</table>
