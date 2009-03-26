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

?>
	<style type="text/css">
	<!--
	/* tab colors */
	.tab                { background-color : #ffffff; }
	.tab_s              { background-color : #5984C3; }
	.tab_u              { background-color : #A0B7C9; }
	
	/* tab link colors */
	a.tabLink           { text-decoration : none; }
	a.tabLink:link      { text-decoration : none; }
	a.tabLink:visited   { text-decoration : none; }
	a.tabLink:hover     { text-decoration : underline; }
	a.tabLink:active    { text-decoration : underline; }
	
	/* tab link size */
	p.tabLink_s         { color: navy; font-size : 10pt; font-weight : bold; padding : 0 8px 1px 2px; margin : 0; }
	p.tabLink_u         { color: black; font-size : 10pt; padding : 0 8px 1px 2px; margin : 0; }
	
	/* text styles */
	.strike 	       { text-decoration: line-through; }
	.bold              { font-weight: bold; }
	.newstitle         { font-weight: bold; color: purple; }
	.title_group       { font-size: 16px; font-weight: bold; color: #5984C3; text-decoration: none; }
	.bluetitle:visited { color: #323fa2; text-decoration: none; }
	
	.Stil1 {font-size: xx-small}
	.Stil2 {font-size: x-small}
	.Stil3 {color: #FFFFFF}
	.Stil4 {font-size: xx-small; color: #FFFFFF; }
	
	-->
	</style>
		<table align="center" border="0" cellpadding="0" cellspacing="0" width="100%">
		  <tr align="left" valign="top">
			<!-- title -->
			<td valign="bottom" width="100%">
			  <table border="0" cellpadding="0" cellspacing="0" width="100%">
				<tr>
				  <td class="title_group" nowrap="nowrap"><?php 
				  
				  if ($RSDB_SET_rank2 == "new" || $RSDB_SET_rank2 == "") { 
				  	echo "New";
				  }
				  else if ($RSDB_SET_rank2 == "awards") { 
				  	echo "Awards";
				  }
				  else if ($RSDB_SET_rank2 == "ratings") { 
				  	echo "User Ratings";
				  }
				  else if ($RSDB_SET_rank2 == "forums") { 
				  	echo "Forums";
				  }
				  else if ($RSDB_SET_rank2 == "screenshots") { 
				  	echo "Screenshots";
				  }
				  else if ($RSDB_SET_rank2 == "vendors") { 
				  	echo "Vendors";
				  }
				  
				  ?></td>
				</tr>
				<tr valign="bottom">
				  <td class="tab_s"><img src="images/white_pixel.gif" alt="" height="1" width="1"></td>
				</tr>
			</table></td>

          <!-- start tab -->
          <td nowrap="nowrap">
            <table border="0" cellpadding="0" cellspacing="0">
                <tr align="left" valign="top">
                  <td width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="4"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "new" || $RSDB_SET_rank2 == "") { echo "tab_s"; } else { echo "tab_u"; } ?>"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "new" || $RSDB_SET_rank2 == "") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr align="left" valign="top">
                  <td class="<?php if ($RSDB_SET_rank2 == "new" || $RSDB_SET_rank2 == "") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/white_pixel.gif" alt="" height="4" width="1"></td>
                  <td width="4"><img src="images/tab_corner_<?php if ($RSDB_SET_rank2 == "new" || $RSDB_SET_rank2 == "") { echo "active"; } else { echo "inactive"; } ?>.gif" alt="" height="4" width="4"></td>
                  <td><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "new" || $RSDB_SET_rank2 == "") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr valign="middle">
                  <td class="<?php if ($RSDB_SET_rank2 == "new" || $RSDB_SET_rank2 == "") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="4"><img src="images/blank.gif" alt="" height="1" width="4"></td>
                  <td nowrap="nowrap"><p class="<?php if ($RSDB_SET_rank2 == "new" || $RSDB_SET_rank2 == "") { echo "tabLink_s"; } else { echo "tabLink_u"; } ?>"><a href="<?php echo $RSDB_intern_link_rank_rank2; ?>new" class="tabLink">New</a></p></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "new" || $RSDB_SET_rank2 == "") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr valign="bottom">
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "new" || $RSDB_SET_rank2 == "") { echo "tab"; } else { echo "tab_s"; } ?>" width="4"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "new" || $RSDB_SET_rank2 == "") { echo "tab"; } else { echo "tab_s"; } ?>"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="tab_s" width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
          </table></td>
          <!-- end tab -->

          <!-- start tab -->
          <td nowrap="nowrap">
            <table border="0" cellpadding="0" cellspacing="0">
                <tr align="left" valign="top">
                  <td width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="4"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "awards") { echo "tab_s"; } else { echo "tab_u"; } ?>"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "awards") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr align="left" valign="top">
                  <td class="<?php if ($RSDB_SET_rank2 == "awards") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/white_pixel.gif" alt="" height="4" width="1"></td>
                  <td width="4"><img src="images/tab_corner_<?php if ($RSDB_SET_rank2 == "awards") { echo "active"; } else { echo "inactive"; } ?>.gif" alt="" height="4" width="4"></td>
                  <td><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "awards") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr valign="middle">
                  <td class="<?php if ($RSDB_SET_rank2 == "awards") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="4"><img src="images/blank.gif" alt="" height="1" width="4"></td>
                  <td nowrap="nowrap"><p class="<?php if ($RSDB_SET_rank2 == "awards") { echo "tabLink_s"; } else { echo "tabLink_u"; } ?>"><a href="<?php echo $RSDB_intern_link_rank_rank2; ?>awards" class="tabLink">Awards</a></p></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "awards") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr valign="bottom">
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "awards") { echo "tab"; } else { echo "tab_s"; } ?>" width="4"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "awards") { echo "tab"; } else { echo "tab_s"; } ?>"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="tab_s" width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
          </table></td>
          <!-- end tab -->

          <!-- start tab -->
          <td nowrap="nowrap">
            <table border="0" cellpadding="0" cellspacing="0">
                <tr align="left" valign="top">
                  <td width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="4"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "ratings") { echo "tab_s"; } else { echo "tab_u"; } ?>"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "ratings") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr align="left" valign="top">
                  <td class="<?php if ($RSDB_SET_rank2 == "ratings") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/white_pixel.gif" alt="" height="4" width="1"></td>
                  <td width="4"><img src="images/tab_corner_<?php if ($RSDB_SET_rank2 == "ratings") { echo "active"; } else { echo "inactive"; } ?>.gif" alt="" height="4" width="4"></td>
                  <td><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "ratings") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr valign="middle">
                  <td class="<?php if ($RSDB_SET_rank2 == "ratings") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="4"><img src="images/blank.gif" alt="" height="1" width="4"></td>
                  <td nowrap="nowrap"><p class="<?php if ($RSDB_SET_rank2 == "ratings") { echo "tabLink_s"; } else { echo "tabLink_u"; } ?>"><a href="<?php echo $RSDB_intern_link_rank_rank2; ?>ratings" class="tabLink">User Ratings</a></p></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "ratings") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr valign="bottom">
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "ratings") { echo "tab"; } else { echo "tab_s"; } ?>" width="4"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "ratings") { echo "tab"; } else { echo "tab_s"; } ?>"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="tab_s" width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
          </table></td>
          <!-- end tab -->
		  
          <!-- start tab -->
          <td nowrap="nowrap">
            <table border="0" cellpadding="0" cellspacing="0">
                <tr align="left" valign="top">
                  <td width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="4"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "forums") { echo "tab_s"; } else { echo "tab_u"; } ?>"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "forums") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr align="left" valign="top">
                  <td class="<?php if ($RSDB_SET_rank2 == "forums") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/white_pixel.gif" alt="" height="4" width="1"></td>
                  <td width="4"><img src="images/tab_corner_<?php if ($RSDB_SET_rank2 == "forums") { echo "active"; } else { echo "inactive"; } ?>.gif" alt="" height="4" width="4"></td>
                  <td><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "forums") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr valign="middle">
                  <td class="<?php if ($RSDB_SET_rank2 == "forums") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="4"><img src="images/blank.gif" alt="" height="1" width="4"></td>
                  <td nowrap="nowrap"><p class="<?php if ($RSDB_SET_rank2 == "forums") { echo "tabLink_s"; } else { echo "tabLink_u"; } ?>"><a href="<?php echo $RSDB_intern_link_rank_rank2; ?>forums" class="tabLink">Forums</a></p></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "forums") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr valign="bottom">
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "forums") { echo "tab"; } else { echo "tab_s"; } ?>" width="4"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "forums") { echo "tab"; } else { echo "tab_s"; } ?>"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="tab_s" width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
          </table></td>
          <!-- end tab -->
		  
          <!-- start tab -->
          <td nowrap="nowrap">
            <table border="0" cellpadding="0" cellspacing="0">
                <tr align="left" valign="top">
                  <td width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="4"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "screenshots") { echo "tab_s"; } else { echo "tab_u"; } ?>"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "screenshots") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr align="left" valign="top">
                  <td class="<?php if ($RSDB_SET_rank2 == "screenshots") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/white_pixel.gif" alt="" height="4" width="1"></td>
                  <td width="4"><img src="images/tab_corner_<?php if ($RSDB_SET_rank2 == "screenshots") { echo "active"; } else { echo "inactive"; } ?>.gif" alt="" height="4" width="4"></td>
                  <td><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "screenshots") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr valign="middle">
                  <td class="<?php if ($RSDB_SET_rank2 == "screenshots") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="4"><img src="images/blank.gif" alt="" height="1" width="4"></td>
                  <td nowrap="nowrap"><p class="<?php if ($RSDB_SET_rank2 == "screenshots") { echo "tabLink_s"; } else { echo "tabLink_u"; } ?>"><a href="<?php echo $RSDB_intern_link_rank_rank2; ?>screenshots" class="tabLink">Screenshots</a></p></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "screenshots") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr valign="bottom">
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "screenshots") { echo "tab"; } else { echo "tab_s"; } ?>" width="4"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "screenshots") { echo "tab"; } else { echo "tab_s"; } ?>"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="tab_s" width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
          </table></td>
          <!-- end tab -->		

          <!-- start tab -->
          <td nowrap="nowrap">
            <table border="0" cellpadding="0" cellspacing="0">
                <tr align="left" valign="top">
                  <td width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="4"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "vendors") { echo "tab_s"; } else { echo "tab_u"; } ?>"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "vendors") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr align="left" valign="top">
                  <td class="<?php if ($RSDB_SET_rank2 == "vendors") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/white_pixel.gif" alt="" height="4" width="1"></td>
                  <td width="4"><img src="images/tab_corner_<?php if ($RSDB_SET_rank2 == "vendors") { echo "active"; } else { echo "inactive"; } ?>.gif" alt="" height="4" width="4"></td>
                  <td><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "vendors") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr valign="middle">
                  <td class="<?php if ($RSDB_SET_rank2 == "vendors") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="4"><img src="images/blank.gif" alt="" height="1" width="4"></td>
                  <td nowrap="nowrap"><p class="<?php if ($RSDB_SET_rank2 == "vendors") { echo "tabLink_s"; } else { echo "tabLink_u"; } ?>"><a href="<?php echo $RSDB_intern_link_rank_rank2; ?>vendors" class="tabLink">Vendors</a></p></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "vendors") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr valign="bottom">
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "vendors") { echo "tab"; } else { echo "tab_s"; } ?>" width="4"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_rank2 == "vendors") { echo "tab"; } else { echo "tab_s"; } ?>"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="tab_s" width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
          </table></td>
          <!-- end tab -->		
		  
			<!-- fill the remaining space -->
			<td valign="bottom" width="10">
			  <table border="0" cellpadding="0" cellspacing="0" width="100%">
				<tr valign="bottom">
				  <td class="tab_s"><img src="images/white_pixel.gif" alt="" height="1" width="10"></td>
				</tr>
			</table></td>
		  </tr>
		</table>
<br />
<br />

<?php 
if ($RSDB_SET_rank2 == "new" || $RSDB_SET_rank2 == "") { 
?>
<h3>Recent submissions<font color="#B5E196"></font></h3>
<p>There are <a href="<?php echo $RSDB_intern_link_db_sec; ?>stats"><b> 
  <?php

  $stmt=CDBConnection::getInstance()->prepare("SELECT COUNT(*) FROM rsdb_groups WHERE grpentr_visible = '1' AND grpentr_comp = '1'");
  $stmt->execute();
	$result_count_cat = $stmt->fetch(PDO::FETCH_NUM);
	echo $result_count_cat[0];


?> applications and drivers</b></a> currently in the database.</p>


<div style="margin:0; margin-top:10px; margin-right:10px; border:1px solid #dfdfdf; padding:0em 1em 1em 1em; background-color:#EAF0F8;"> <br />
    <table width="100%"  border="0">
      <tr valign="top">
        <td width="48%"><h4>Application entries</h4><table width="100%" border="0" cellpadding="1" cellspacing="1">
            <tr bgcolor="#5984C3">
              <td width="15%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Time</strong></font></div></td>
              <td width="50%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Application</strong></font></div></td>
              <td width="35%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>ReactOS</strong></font></div></td>
            </tr>
            <?php 
		$cellcolor1="#E2E2E2";
		$cellcolor2="#EEEEEE";
		$cellcolorcounter="0";

  $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp WHERE comp_visible = '1' ORDER BY comp_id DESC LIMIT 5");
  $stmt->execute();
	while($result_date_entry_records = $stmt->fetch(PDO::FETCH_ASSOC)) {
?>
            <tr bgcolor="<?php
									$cellcolorcounter++;
									if ($cellcolorcounter == "1") {
										echo $cellcolor1;
										$farbe = $cellcolor1;
									}
									elseif ($cellcolorcounter == "2") {
										$cellcolorcounter="0";
										echo $cellcolor2;
										$farbe = $cellcolor2;
									}
								 ?>">
              <td><div align="center"><font size="1" face="Arial, Helvetica, sans-serif">
                  <?php
		echo $result_date_entry_records['comp_date'];

    ?>
              </font></div></td>
              <td><font size="2" face="Arial, Helvetica, sans-serif"> &nbsp;
                    <?php
		echo "<b><a href=\"". $RSDB_intern_link_item_comp.$result_date_entry_records['comp_id']."\">".$result_date_entry_records['comp_name']."</a></b>";

    ?>
              </font></td>
              <td><font size="2" face="Arial, Helvetica, sans-serif"> &nbsp;
                    <?php
		echo "ReactOS ".@show_osversion($result_date_entry_records['comp_osversion']);

    ?>
              </font></td>
            </tr>
            <?php 
	}
?>
        </table></td>
        <td width="4%">&nbsp;</td>
        <td width="48%"><h4>Compatibility Test Reports</h4><table width="100%" border="0" cellpadding="1" cellspacing="1">
            <tr bgcolor="#5984C3">
              <td width="15%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Time</strong></font></div></td>
              <td width="50%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Application</strong></font></div></td>
              <td width="35%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Function</strong></font></div></td>
            </tr>
            <?php 
		$cellcolor1="#E2E2E2";
		$cellcolor2="#EEEEEE";
		$cellcolorcounter="0";

  $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp_testresults WHERE test_visible = '1' ORDER BY test_id DESC LIMIT 5");
  $stmt->execute();
	while($result_date_entry_records = $stmt->fetch(PDO::FETCH_ASSOC)) {
?>
            <tr bgcolor="<?php
									$cellcolorcounter++;
									if ($cellcolorcounter == "1") {
										echo $cellcolor1;
										$farbe = $cellcolor1;
									}
									elseif ($cellcolorcounter == "2") {
										$cellcolorcounter="0";
										echo $cellcolor2;
										$farbe = $cellcolor2;
									}
								 ?>">
              <td><div align="center"><font size="1" face="Arial, Helvetica, sans-serif">
                  <?php
		echo $result_date_entry_records['test_user_submit_timestamp'];

    ?>
              </font></div></td>
              <td><font size="2" face="Arial, Helvetica, sans-serif"> &nbsp;
                    <?php
    $stmt_sub=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp WHERE comp_id = :comp_id LIMIT 1");
    $stmt_sub->bindParam('comp_id',$result_date_entry_records['test_comp_id'],PDO::PARAM_STR);
    $stmt_sub->execute();
		$result_date_vendor = $stmt_sub->fetch(PDO::FETCH_ASSOC);
		echo "<b><a href=\"". $RSDB_intern_link_item_comp.$result_date_vendor['comp_id'] ."&amp;item2=tests\">".$result_date_vendor['comp_name']."</a></b>";

    ?>
              </font></td>
              <td><div align="left"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;
                        <?php 
		echo draw_stars_small($result_date_entry_records['test_result_function'], 1, 5, "");
	?>
              </font></div></td>
            </tr>
            <?php 
	}
?>
        </table></td>
      </tr>
      <tr valign="top">
        <td>&nbsp;</td>
        <td>&nbsp;</td>
        <td>&nbsp;</td>
      </tr>
      <tr valign="top">
        <td><h4>Forum entries</h4><table width="100%" border="0" cellpadding="1" cellspacing="1">
          <tr bgcolor="#5984C3">
            <td width="15%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Time</strong></font></div></td>
            <td width="50%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Topic</strong></font></div></td>
            <td width="35%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Application</strong></font></div></td>
          </tr>
          <?php 
		$cellcolor1="#E2E2E2";
		$cellcolor2="#EEEEEE";
		$cellcolorcounter="0";

 
  $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp_forum WHERE fmsg_visible = '1' ORDER BY fmsg_id DESC LIMIT 5");
  $stmt->execute();
	while($result_date_entry_records = $stmt->fetch(PDO::FETCH_ASSOC)) {
?>
          <tr bgcolor="<?php
									$cellcolorcounter++;
									if ($cellcolorcounter == "1") {
										echo $cellcolor1;
										$farbe = $cellcolor1;
									}
									elseif ($cellcolorcounter == "2") {
										$cellcolorcounter="0";
										echo $cellcolor2;
										$farbe = $cellcolor2;
									}
								 ?>">
            <td><div align="center"><font size="1" face="Arial, Helvetica, sans-serif">
                <?php
		echo $result_date_entry_records['fmsg_date'];

    ?>
            </font></div></td>
            <td><font size="2" face="Arial, Helvetica, sans-serif"> &nbsp;
                  <?php

    $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp WHERE comp_id = :comp_id LIMIT 1");
    $stmt->bindParam('comp_id',$result_date_entry_records['fmsg_comp_id'],PDO::PARAM_STR);
    $stmt->execute();
		$result_date_vendor = $stmt->fetch(PDO::FETCH_ASSOC);

		echo "<b><a href=\"". $RSDB_intern_link_item_comp.$result_date_vendor['comp_id'] ."&amp;item2=forum&amp;fstyle=fthreads&amp;msg=". urlencode($result_date_entry_records['fmsg_id']) ."\">".$result_date_entry_records['fmsg_subject']."</a></b>";

    ?>
            </font></td>
            <td><div align="left"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;
                      <?php
		echo "<a href=\"". $RSDB_intern_link_item_comp.$result_date_vendor['comp_id'] ."&amp;item2=tests\">".$result_date_vendor['comp_name']."</a>";

    ?>
            </font></div></td>
          </tr>
          <?php 
	}
?>
        </table></td>
        <td>&nbsp;</td>
        <td><h4>Screenshots</h4>
          <table width="100%" border="0" cellpadding="1" cellspacing="1">
            <tr bgcolor="#5984C3">
              <td width="15%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Time</strong></font></div></td>
              <td width="50%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Description</strong></font></div></td>
              <td width="35%"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Application</strong></font></div></td>
            </tr>
            <?php 
		$cellcolor1="#E2E2E2";
		$cellcolor2="#EEEEEE";
		$cellcolorcounter="0";

 
  $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_object_media WHERE media_visible = '1' ORDER BY media_id DESC LIMIT 5");
  $stmt->execute();
	while($result_date_entry_records = $stmt->fetch(PDO::FETCH_ASSOC)) {
?>
            <tr bgcolor="<?php
									$cellcolorcounter++;
									if ($cellcolorcounter == "1") {
										echo $cellcolor1;
										$farbe = $cellcolor1;
									}
									elseif ($cellcolorcounter == "2") {
										$cellcolorcounter="0";
										echo $cellcolor2;
										$farbe = $cellcolor2;
									}
								 ?>">
              <td><div align="center"><font size="1" face="Arial, Helvetica, sans-serif">
                  <?php
		echo $result_date_entry_records['media_date'];

    ?>
              </font></div></td>
              <td><font size="2" face="Arial, Helvetica, sans-serif"> &nbsp;
                    <?php

    $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp WHERE comp_media = :group_id LIMIT 1");
    $stmt->bindParam('media',$result_date_entry_records['media_groupid'],PDO::PARAM_STR);
    $stmt->execute();
		$result_date_vendor = $stmt->fetch(PDO::FETCH_ASSOC);
		echo "<b><a href=\"". $RSDB_intern_link_item_comp.$result_date_vendor['comp_id'] ."&amp;item2=screens&amp;entry=". urlencode($result_date_entry_records['media_id']) ."\">".htmlentities($result_date_entry_records['media_description'])."</a></b>";

    ?>
              </font></td>
              <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;
                    <?php
		echo "<a href=\"". $RSDB_intern_link_item_comp.$result_date_vendor['comp_id'] ."&amp;item2=screens\">".$result_date_vendor['comp_name']."</a>";

    ?>
              </font></td>
            </tr>
            <?php 
	}
?>
          </table></td>
      </tr>
    </table>
</div>
<?php 
}
else if ($RSDB_SET_rank2 == "awards" || $RSDB_SET_rank2 == "ratings") { 
	if ($RSDB_SET_rank2 == "ratings") { 
		echo "<p>Under construction ...</p>";
	}
	/*echo '<p>Filter: ';
	if ($RSDB_SET_filter == "active") {	
		echo '<b>active users</b>';
		$RSDB_intern_users_filt = "WHERE `user_account_enabled` = 'yes'";
	}
	else {
		echo '<a href="'.RSDB_intern_link_rank_filter.'">all symbols</a>';
	}
	echo ' | ';
	if ($RSDB_SET_filter == "all") {	
		echo '<b>all users</b>';
		$RSDB_intern_users_filt = "WHERE `user_account_enabled` != ''";
	}
	else {
		echo '<a href="'.$RSDB_intern_link_rank_filter.'">all users</a>';
	}
	echo '</p>';*/


	if ($RSDB_SET_letter == "all") {
		$RSDB_SET_letter = "%";
	}
	
	$stmt=CDBConnection::getInstance()->prepare("SELECT COUNT(*) FROM ( SELECT grpentr_id, MAX(i.comp_award) AS derived_max, g.grpentr_vendor, g.grpentr_name FROM rsdb_groups g JOIN rsdb_item_comp i ON i.comp_groupid = g.grpentr_id AND g.grpentr_visible = '1' AND g.grpentr_comp = '1' GROUP BY grpentr_id) v1 ORDER BY v1.derived_max DESC");
  $stmt->execute();
	$result_count_cat = $stmt->fetch(PDO::FETCH_NUM);
	if ($result_count_cat[0]) {
	
		echo "<p align='center'>";
		$j=0;
		for ($i=0; $i < $result_count_cat[0]; $i += $RSDB_intern_items_per_page) {
			$j++;
			if ($RSDB_SET_curpos == $i) {
				echo "<b>".$j."</b> ";
			}
			else {
				echo "<a href='".$RSDB_intern_link_rank_curpos.$i."'>".$j."</a> ";
			}
		}
		$j=0;
		echo "</p>";
	
?> 
	<table width="100%" border="0" cellpadding="1" cellspacing="1">
	  <tr bgcolor="#5984C3"> 
		<td width="25%" bgcolor="#5984C3"> 
		  <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif" ><strong>Application</strong></font></div></td>
		<td width="15%" title="Version">
          <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Vendor</strong></font></div></td>
		<td width="14%"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif" ><strong>Award</strong></font></div></td>
		<td width="19%">
          <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Function</strong></font></div></td>
		<td width="19%">
		  <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Install</strong></font></div></td>
		<td width="8%" title="Status"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Status</strong></font></div></td>
	  </tr>
	  <?php
		
		if ($RSDB_SET_rank2 == "awards") {
      $stmt=CDBConnection::getInstance()->prepare("SELECT v1.grpentr_id, v1.derived_max, v1.grpentr_vendor, v1.grpentr_name FROM ( SELECT grpentr_id, MAX(i.comp_award) derived_max, g.grpentr_vendor, g.grpentr_name FROM rsdb_groups g  JOIN rsdb_item_comp i ON i.comp_groupid = g.grpentr_id AND g.grpentr_visible = '1' AND g.grpentr_comp = '1' GROUP BY grpentr_id) v1 ORDER BY v1.derived_max DESC LIMIT :limit OFFSET :offset");
		}
		else if ($RSDB_SET_rank2 == "ratings") {
      $stmt=CDBConnection::getInstance()->prepare("SELECT v1.grpentr_id, v1.derived_max, v1.grpentr_vendor, v1.grpentr_name FROM (SELECT grpentr_id, MAX(i.comp_award) derived_max, g.grpentr_vendor, g.grpentr_name FROM rsdb_groups g JOIN rsdb_item_comp i ON i.comp_groupid = g.grpentr_id AND g.grpentr_visible = '1' AND g.grpentr_comp = '1' GROUP BY grpentr_id) v1 ORDER BY v1.derived_max DESC LIMIT :limit OFFSET :offset");
		}
    $stmt->bindParam('limit',$RSDB_intern_items_per_page,PDO::PARAM_INT);
    $stmt->bindParam('offset',$RSDB_SET_curpos,PDO::PARAM_INT);
    $stmt->execute();

			$farbe1="#E2E2E2";
			$farbe2="#EEEEEE";
			$zaehler="0";
			//$farbe="#CCCCC";
			
			while($result_page = $stmt->fetch(PDO::FETCH_ASSOC)) { // Pages
		?>
	  <tr> 
		<td valign="top" bgcolor="<?php
										$zaehler++;
										if ($zaehler == "1") {
											echo $farbe1;
											$farbe = $farbe1;
										}
										elseif ($zaehler == "2") {
											$zaehler="0";
											echo $farbe2;
											$farbe = $farbe2;
										}
									 ?>" > <div align="left"><font face="Arial, Helvetica, sans-serif">&nbsp;<a href="<?php echo $RSDB_intern_link_rank2_group.$result_page['grpentr_id']; ?>"><b><?php echo htmlentities($result_page['grpentr_name']); ?></b></a></font></div></td>
		<td valign="top" bgcolor="<?php echo $farbe; ?>"><div align="left"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;
        <?php 

				$stmt_sub=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_vendor WHERE vendor_id = :vendor_id");
        $stmt_sub->bindParam('vendor_id',$result_page['grpentr_vendor'],PDO::PARAM_STR);
        $stmt_sub->execute();
				$result_entry_vendor = $stmt->fetch(PDO::FETCH_ASSOC);
				echo '<a href="'.$RSDB_intern_link_vendor_sec.$result_entry_vendor['vendor_id'].'">'.$result_entry_vendor['vendor_name'].'</a>';
				
			?>
        </font></div></td>
		<td valign="top" bgcolor="<?php echo $farbe; ?>"><div align="left"><font size="1" face="Arial, Helvetica, sans-serif">			&nbsp;<img src="media/icons/awards/<?php echo draw_award_icon($result_page["derived_max"]); ?>.gif" alt="<?php echo draw_award_name($result_page["derived_max"]); ?>" width="16" height="16" /> <?php echo draw_award_name($result_page["derived_max"]); ?>	</font></div></td>
		<td valign="top" bgcolor="<?php echo $farbe; ?>"><div align="left"><font size="2">
            </font><font size="1" face="Arial, Helvetica, sans-serif">
            <?php 
				$counter_stars_install_sum = 0;
				$counter_stars_function_sum = 0;
				$counter_stars_user_sum = 0;
				
				$counter_items = 0;
	
				$counter_testentries = 0;
				$counter_forumentries = 0;
				$counter_screenshots = 0;
	
        $stmt_sub=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp WHERE comp_groupid = :group_id AND comp_visible = '1' ORDER BY comp_groupid DESC");
        $stmt_sub->bindParam('group_id',$result_page['grpentr_id'],PDO::PARAM_STR);
        $stmt_sub->execute();
				while($result_page = $stmt_sub->fetch(PDO::FETCH_ASSOC)) { 
					$counter_items++;
          $stmt_count=CDBConnection::getInstance()->prepare("SELECT COUNT(*) AS user_sum, SUM(test_result_install) AS install_sum, SUM(test_result_function) AS function_sum FROM rsdb_item_comp_testresults WHERE test_visible = '1' AND test_comp_id = :comp_id");
          $stmt_count->bindParam('comp_id',$result_page['comp_id'],PDO::PARAM_STR);
          $stmt_count->execute();
          $tmp=$stmt_count->fetch(PDO::FETCH_ASSOC);

          $counter_stars_install_sum += $tmp['install_sum'];
          $counter_stars_function_sum += $tmp['function_sum'];
          $counter_stars_user_sum += $tmp['user_sum'];
					
          $stmt_count=CDBConnection::getInstance()->prepare("SELECT COUNT(*) FROM rsdb_item_comp_testresults WHERE test_visible = '1' AND test_comp_id = :comp_id");
          $stmt_count->bindParam('comp_id',$result_page['comp_id'],PDO::PARAM_STR);
          $stmt_count->execute();
					$result_count_testentries = $stmt_count->fetch(PDO::FETCH_NUM);
					$counter_testentries += $result_count_testentries[0];
					
					// Forum entries:
          $stmt_count=CDBConnection::getInstance()->prepare("SELECT COUNT(*) FROM rsdb_item_comp_forum WHERE fmsg_visible = '1' AND fmsg_comp_id = :comp_id");
          $stmt_count->bindParam('comp_id',$result_page['comp_id'],PDO::PARAM_STR);
          $stmt_count->execute();
					$result_count_forumentries = $stmt_count->fetch(PDO::FETCH_NUM);
					$counter_forumentries += $result_count_forumentries[0];
	
					// Screenshots:
          $stmt_count=CDBConnection::getInstance()->prepare("SELECT COUNT(*) FROM rsdb_object_media WHERE media_visible = '1' AND media_groupid = :group_id");
          $stmt_count->bindParam('group_id',$result_page['comp_media'],PDO::PARAM_STR);
          $stmt_count->execute();
					$result_count_screenshots = $stmt_count->fetch(PDO::FETCH_NUM);
					$counter_screenshots += $result_count_screenshots[0];
				}
				
		?>
            </font><font size="2">
            <?php 
				
				echo draw_stars_small($counter_stars_function_sum, $counter_stars_user_sum, 5, "") . " (".$counter_stars_user_sum.")";
				
				?>
            </font></div></td>
		<td valign="top" bgcolor="<?php echo $farbe; ?>"><div align="left"><font size="2">
			<?php 
				
				echo draw_stars_small($counter_stars_function_sum, $counter_stars_user_sum, 5, "") . " (".$counter_stars_user_sum.")";
				
				?>
		</font></div></td>
		<td valign="top" bgcolor="<?php echo $farbe; ?>" title="<?php echo "Tests: ".$counter_testentries.", Forum entries: ".$counter_forumentries.", Screenshots: ".$counter_screenshots; ?>"><div align="center">
			<table width="100%" border="0" cellpadding="1" cellspacing="1">
			  <tr>
				<td width="33%"><div align="center">
					<?php if ($counter_testentries > 0) { ?>
					<img src="media/icons/info/test.gif" alt="Compatibility Test Report entries" width="13" height="13">
					<?php } else { echo "&nbsp;"; } ?>
				</div></td>
				<td width="33%"><div align="center">
					<?php if ($counter_forumentries > 0) { ?>
					<img src="media/icons/info/forum.gif" alt="Forum entries" width="13" height="13">
					<?php } else { echo "&nbsp;"; } ?>
				</div></td>
				<td width="33%"><div align="center">
					<?php if ($counter_screenshots > 0) { ?>
					<img src="media/icons/info/screenshot.gif" alt="Screenshots" width="13" height="13">
					<?php } else { echo "&nbsp;"; } ?>
				</div></td>
			  </tr>
			</table>
		</div></td>
	  </tr>
	  <?php	
			}	// end while
		?>
	</table>
	<p align="center"><b><?php
	
		echo ($RSDB_SET_curpos+1)." to ";
	
		if (($RSDB_SET_curpos + $RSDB_intern_items_per_page) > $result_count_cat[0]) {
			echo $result_count_cat[0];
		}
		else {
			echo ($RSDB_SET_curpos + $RSDB_intern_items_per_page);
		}
			
		echo " of ".$result_count_cat[0]; 
		
	?></b></p>
<?php
	}
}
else if ($RSDB_SET_rank2 == "vendors") { 

?>
<p>Under construction ...</p>
<table width="100%" border="0" cellpadding="1" cellspacing="1">
  <tr bgcolor="#5984C3">
    <td width="15%">
      <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif" ><strong>Vendor</strong></font></div></td>
    <td width="30%">
      <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif" ><strong>Fullname</strong></font></div></td>
    <td width="27%">
      <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif" ><strong>Website</strong></font></div></td>
    <td width="18%">
      <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Number</strong></font></div></td>
  </tr>
  <?php
	
		$stmt=CDBConnection::getInstance()->prepare("SELECT v1.vendor_id, v1.derived_max, v1.vendor_name, v1.vendor_url, v1.vendor_fullname FROM ( SELECT vendor_id, MAX(i.grpentr_vendor) derived_max, g.vendor_name, g.vendor_url, g.vendor_fullname FROM rsdb_item_vendor g JOIN rsdb_groups i ON i.grpentr_vendor = g.vendor_id AND g.vendor_visible = '1' GROUP BY vendor_id ) v1 ORDER BY v1.derived_max DESC");
    $stmt->execute();
		
		
	
		$farbe1="#E2E2E2";
		$farbe2="#EEEEEE";
		$zaehler="0";
		//$farbe="#CCCCC";
		
		while($result_page = $stmt->fetch(PDO::FETCH_ASSOC)) { // Pages
	?>
  <tr>
    <td valign="top" bgcolor="<?php
									$zaehler++;
									if ($zaehler == "1") {
										echo $farbe1;
										$farbe = $farbe1;
									}
									elseif ($zaehler == "2") {
										$zaehler="0";
										echo $farbe2;
										$farbe = $farbe2;
									}
								 ?>">
      <div align="left"><font face="Arial, Helvetica, sans-serif">&nbsp;<b><a href="<?php echo $RSDB_intern_link_vendor_sec_comp.$result_page['vendor_id']; ?>"><?php echo $result_page['vendor_name']; ?></a></b></font> </div></td>
    <td valign="top" bgcolor="<?php echo $farbe; ?>"><font size="2"><?php echo $result_page['vendor_fullname']; ?></font></td>
    <td valign="top" bgcolor="<?php echo $farbe; ?>"><div align="left"><font size="2"><a href="<?php echo $result_page['vendor_url']; ?>"><?php echo $result_page['vendor_url']; ?></a></font></div></td>
    <td valign="top" bgcolor="<?php echo $farbe; ?>"><div align="left"><font size="2"><?php echo $result_page['derived_max']; ?> </font></div></td>
  </tr>
  <?php	
		}	// end while
	?>
</table>
<p>&nbsp;</p>
<?php
}
else if ($RSDB_SET_rank2 == "screenshots") {
	echo "<p>Under construction ...</p>";

	$stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_object_media WHERE (( media_useful_vote_value / media_useful_vote_user) > 2 OR  media_useful_vote_user < 5)");
  $stmt->execute();
	$result_count_cat = $stmt->fetch(PDO::FETCH_NUM);
	if ($result_count_cat[0]) {
	
		echo "<p align='center'>";
		$j=0;
		for ($i=0; $i < $result_count_cat[0]; $i += $RSDB_intern_items_per_page) {
			$j++;
			if ($RSDB_SET_curpos == $i) {
				echo "<b>".$j."</b> ";
			}
			else {
				echo "<a href='".$RSDB_intern_link_rank_curpos.$i."'>".$j."</a> ";
			}
		}
		$j=0;
		echo "</p>";
 
		$roscms_TEMP_counter = 0;
		echo '<table width="100%"  border="0" cellpadding="3" cellspacing="1">';
		$stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_object_media WHERE (( media_useful_vote_value / media_useful_vote_user) > 2 OR  media_useful_vote_user < 5) ORDER BY media_order ASC LIMIT :limit OFFSET :offset");
    $stmt->bindParam('limit',$RSDB_intern_items_per_page,PDO::PARAM_INT);
    $stmt->bindParam('offset',$RSDB_SET_curpos,PDO::PARAM_INT);
    $stmt->execute();
		while($result_screenshots= $stmt->fetch(PDO::FETCH_ASSOC)) {
			$roscms_TEMP_counter++;
			if ($roscms_TEMP_counter == 1) {
				echo "<tr>";
			}
			echo '<td width="33%" valign="top">';
	
			echo '<p align="center"><br /><a href="'.$RSDB_intern_link_item_item2.'screens&amp;entry='.$result_screenshots["media_id"].'"><img src="media/files/'.$result_screenshots["media_filetype"].'/'.urlencode($result_screenshots["media_thumbnail"]).'" width="250" height="188" border="0" alt="';
			echo 'Description: '.htmlentities($result_screenshots["media_description"])."\nUser: ".usrfunc_GetUsername($result_screenshots["media_user_id"])."\nDate: ".$result_screenshots["media_date"]."\n\n".htmlentities($result_screenshots["media_exif"]);
			echo '"></a><br /><i>'.htmlentities($result_screenshots["media_description"]).'</i><br />';
			echo '<br /><font size="1">';
				  
					$RSDB_TEMP_voting_history = strchr($result_screenshots['media_useful_vote_user_history'],("|".$RSDB_intern_user_id."="));
					if ($RSDB_TEMP_voting_history == false) {
						echo "Rate this screenshot: ";
						if ($result_screenshots['media_useful_vote_user'] > $RSDB_setting_stars_threshold) {
							echo draw_stars_vote($result_screenshots['media_useful_vote_value'], $result_screenshots['media_useful_vote_user'], 5, "", ($RSDB_intern_link_item_item2_vote.$result_screenshots['media_id']."&amp;vote2="));
						}
						else {
							echo draw_stars_vote(0, 0, 5, "", ($RSDB_intern_link_item_item2_vote.$result_screenshots['media_id']."&amp;vote2="));
						}
					}
					else {
						echo "Rating: ";
						echo draw_stars($result_screenshots['media_useful_vote_value'], $result_screenshots['media_useful_vote_user'], 5, "");
					}
					
			echo '</font><br /><br /></p>';
	
			echo "</td>";
			if ($roscms_TEMP_counter == 3) {
				echo "</tr>";
				$roscms_TEMP_counter = 0;
			}
		}
		
		if ($roscms_TEMP_counter == 1) {
			echo '<td width="33%" valign="top">&nbsp;</td>';
			echo '<td width="33%" valign="top">&nbsp;</td></tr>';
		}
		if ($roscms_TEMP_counter == 2) {
			echo '<td width="33%" valign="top">&nbsp;</td></tr>';
		}
	
		echo "</table>";

		?>
	<p align="center"><b><?php
	
		echo ($RSDB_SET_curpos+1)." to ";
	
		if (($RSDB_SET_curpos + $RSDB_intern_items_per_page) > $result_count_cat[0]) {
			echo $result_count_cat[0];
		}
		else {
			echo ($RSDB_SET_curpos + $RSDB_intern_items_per_page);
		}
			
		echo " of ".$result_count_cat[0]; 
		
	?></b></p>
<?php
	}
}
else {
	echo "<p>Under construction ...</p>";
}
?>
