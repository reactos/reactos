<?php
    /*
    CompatDB - ReactOS Compatability Database
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


/**
 * class Message
 * 
 */
class Message
{



  /**
   * @FILLME
   *
   * @access public
   */
  public static 	function show($msgtext) { // show the message bar
	echo '<table width="450" border="0" align="center" cellpadding="0" cellspacing="0" class="message">';
	echo '  <tr>';
	echo '    <td><div align="center">'.$msgtext.'</div></td>';
	echo '  </tr>';
	echo '</table>';

	} // end of member function show



  /**
   * @FILLME
   *
   * @access public
   */
  public static 	function noScript() {
    global $RSDB_intern_link_language,$rpm_lang;
    echo '<noscript>
    Your browser does not support JavaScript/JScript or (more likely) you have ECMAScript/JavaScript/JScript<br />
    disabled in your browser security option.<br /><br />
    To gain the best user interface experience please enable ECMAScript/JavaScript/JScript in your browser,<br />
    if possible.<br /><br />
    You will also be able to use the ReactOS Support Database without ECMAScript/JavaScript/JScript.<br /><br />
    <a href="'.$RSDB_intern_link_language.$rpm_lang.'"&amp;ajax=false"><b>Please, click here to disable ECMAScript/JavaScript/JScript features</b></a> (DHTML & Ajax) on this page<br />
    and then you will be able to use all feature even without ECMAScript/JavaScript/JScript enabled!
    </noscript>';
  } // end of member function noScript



  /**
   * @FILLME
   *
   * @access public
   */
  public static 	function loginRequired() {
	global $RSDB_intern_path_server;
	global $RSDB_intern_loginsystem_path;
	
	self::show("<b>Login required!</b><br /><br />Please use the <a href=\"".$RSDB_intern_path_server.$RSDB_intern_loginsystem_path."?page=login&amp;target=%2Fsupport%2F\">login function</a> to get access or 
				<a href=\"".$RSDB_intern_path_server.$RSDB_intern_loginsystem_path."?page=register\">register an account</a> for free!");
  } // end of member function noScript



} // end of Message
?>
