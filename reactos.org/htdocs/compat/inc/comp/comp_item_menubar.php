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

-->
</style>

	<table align="center" border="0" cellpadding="0" cellspacing="0" width="100%">
        <tr align="left" valign="top">
          <!-- title -->
          <td valign="bottom" width="100%">
            <table border="0" cellpadding="0" cellspacing="0" width="100%">
                <tr>
                  <td class="title_group" nowrap="nowrap"><?php 
				  
          if (isset($_GET['item2'])) {
				    if ($_GET['item2'] == 'details' || $_GET['item2'] == '') { 
				    	echo "Overview";
				    }
				    elseif ($_GET['item2'] == 'screens') { 
				    	echo "Screenshots";
				    }
				    elseif ($_GET['item2'] == 'tests') { 
				    	echo "Compatibility Test Reports";
				    }
				    elseif ($_GET['item2'] == 'forum') { 
				    	echo "Forum";
				    }
				    elseif ($_GET['item2'] == 'bugs') { 
				    	echo "Known Bugs";
				    }
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
                  <td class="<?php if (isset($_GET['item2']) && ($_GET['item2'] == 'details' || $_GET['item2'] == '')) { echo "tab_s"; } else { echo "tab_u"; } ?>"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if (isset($_GET['item2']) && ($_GET['item2'] == 'details' || $_GET['item2'] == '')) { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr align="left" valign="top">
                  <td class="<?php if (isset($_GET['item2']) && ($_GET['item2'] == 'details' || $_GET['item2'] == '')) { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/white_pixel.gif" alt="" height="4" width="1"></td>
                  <td width="4"><img src="images/tab_corner_<?php if (isset($_GET['item2']) && ($_GET['item2'] == 'details' || $_GET['item2'] == '')) { echo "active"; } else { echo "inactive"; } ?>.gif" alt="" height="4" width="4"></td>
                  <td><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if (isset($_GET['item2']) && ($_GET['item2'] == 'details' || $_GET['item2'] == '')) { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr valign="middle">
                  <td class="<?php if (isset($_GET['item2']) && ($_GET['item2'] == 'details' || $_GET['item2'] == '')) { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="4"><img src="images/blank.gif" alt="" height="1" width="4"></td>
                  <td nowrap="nowrap"><p class="<?php if (isset($_GET['item2']) && ($_GET['item2'] == 'details' || $_GET['item2'] == '')) { echo "tabLink_s"; } else { echo "tabLink_u"; } ?>"><a href="<?php echo $RSDB_intern_link_item_item2; ?>details" class="tabLink">Overview</a></p></td>
                  <td class="<?php if (isset($_GET['item2']) && ($_GET['item2'] == 'details' || $_GET['item2'] == '')) { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr valign="bottom">
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if (isset($_GET['item2']) && ($_GET['item2'] == 'details' || $_GET['item2'] == '')) { echo "tab"; } else { echo "tab_s"; } ?>" width="4"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if (isset($_GET['item2']) && ($_GET['item2'] == 'details' || $_GET['item2'] == '')) { echo "tab"; } else { echo "tab_s"; } ?>"><img src="images/blank.gif" alt="" height="1" width="1"></td>
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
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'tests') { echo "tab_s"; } else { echo "tab_u"; } ?>"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'tests') { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr align="left" valign="top">
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'tests') { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/white_pixel.gif" alt="" height="4" width="1"></td>
                  <td width="4"><img src="images/tab_corner_<?php if (isset($_GET['item2']) && $_GET['item2'] == 'tests') { echo "active"; } else { echo "inactive"; } ?>.gif" alt="" height="4" width="4"></td>
                  <td><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'tests') { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr valign="middle">
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'tests') { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="4"><img src="images/blank.gif" alt="" height="1" width="4"></td>
                  <td nowrap="nowrap"><p class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'tests') { echo "tabLink_s"; } else { echo "tabLink_u"; } ?>"><a href="<?php echo $RSDB_intern_link_item_item2; ?>tests" class="tabLink">Compatibility</a></p></td>
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'tests') { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr valign="bottom">
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'tests') { echo "tab"; } else { echo "tab_s"; } ?>" width="4"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'tests') { echo "tab"; } else { echo "tab_s"; } ?>"><img src="images/blank.gif" alt="" height="1" width="1"></td>
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
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'forum') { echo "tab_s"; } else { echo "tab_u"; } ?>"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'forum') { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr align="left" valign="top">
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'forum') { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/white_pixel.gif" alt="" height="4" width="1"></td>
                  <td width="4"><img src="images/tab_corner_<?php if (isset($_GET['item2']) && $_GET['item2'] == 'forum') { echo "active"; } else { echo "inactive"; } ?>.gif" alt="" height="4" width="4"></td>
                  <td><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'forum') { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr valign="middle">
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'forum') { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="4"><img src="images/blank.gif" alt="" height="1" width="4"></td>
                  <td nowrap="nowrap"><p class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'forum') { echo "tabLink_s"; } else { echo "tabLink_u"; } ?>"><a href="<?php echo $RSDB_intern_link_item_item2; ?>forum" class="tabLink">Forum</a></p></td>
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'forum') { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr valign="bottom">
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'forum') { echo "tab"; } else { echo "tab_s"; } ?>" width="4"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'forum') { echo "tab"; } else { echo "tab_s"; } ?>"><img src="images/blank.gif" alt="" height="1" width="1"></td>
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
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'screens') { echo "tab_s"; } else { echo "tab_u"; } ?>"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'screens') { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr align="left" valign="top">
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'screens') { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/white_pixel.gif" alt="" height="4" width="1"></td>
                  <td width="4"><img src="images/tab_corner_<?php if (isset($_GET['item2']) && $_GET['item2'] == 'screens') { echo "active"; } else { echo "inactive"; } ?>.gif" alt="" height="4" width="4"></td>
                  <td><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'screens') { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr valign="middle">
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'screens') { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="4"><img src="images/blank.gif" alt="" height="1" width="4"></td>
                  <td nowrap="nowrap"><p class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'screens') { echo "tabLink_s"; } else { echo "tabLink_u"; } ?>"><a href="<?php echo $RSDB_intern_link_item_item2; ?>screens" class="tabLink">Screenshots</a></p></td>
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'screens') { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr valign="bottom">
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'screens') { echo "tab"; } else { echo "tab_s"; } ?>" width="4"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'screens') { echo "tab"; } else { echo "tab_s"; } ?>"><img src="images/blank.gif" alt="" height="1" width="1"></td>
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
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'bugs') { echo "tab_s"; } else { echo "tab_u"; } ?>"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'bugs') { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr align="left" valign="top">
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'bugs') { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/white_pixel.gif" alt="" height="4" width="1"></td>
                  <td width="4"><img src="images/tab_corner_<?php if (isset($_GET['item2']) && $_GET['item2'] == 'bugs') { echo "active"; } else { echo "inactive"; } ?>.gif" alt="" height="4" width="4"></td>
                  <td><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'bugs') { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr valign="middle">
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'bugs') { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="4"><img src="images/blank.gif" alt="" height="1" width="4"></td>
                  <td nowrap="nowrap"><p class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'bugs') { echo "tabLink_s"; } else { echo "tabLink_u"; } ?>"><a href="<?php echo "http://www.reactos.org/bugzilla/buglist.cgi?bug_status=UNCONFIRMED&bug_status=NEW&bug_status=ASSIGNED&bug_status=REOPENED&field0-0-0=product&type0-0-0=substring&value0-0-0=".$result_page['comp_name']."&field0-0-1=component&type0-0-1=substring&value0-0-1=".$result_page['comp_name']."&field0-0-2=short_desc&type0-0-2=substring&value0-0-2=".$result_page['comp_name']."&field0-0-3=status_whiteboard&type0-0-3=substring&value0-0-3=".$result_page['comp_name']; ?>" target="_blank" class="tabLink">Bugs</a></p></td>
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'bugs') { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr valign="bottom">
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'bugs') { echo "tab"; } else { echo "tab_s"; } ?>" width="4"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if (isset($_GET['item2']) && $_GET['item2'] == 'bugs') { echo "tab"; } else { echo "tab_s"; } ?>"><img src="images/blank.gif" alt="" height="1" width="1"></td>
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