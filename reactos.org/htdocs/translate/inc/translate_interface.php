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
	if ( !defined('ROST') )
	{
		die(" ");
	}

?>
<h1>Translate ReactOS</h1> 
<h2>Translate ReactOS</h2> 
<p>ReactOS translation tool</p>
<p>&nbsp;</p>

<?php


	if ($ROST_SET_action == "edit" && $_POST) {
		print_r($_POST);
		echo "<hr />";
		//echo $_POST[1];
		echo "<hr />";
		
		$temo = array () ;
		$temo = $_POST;
		
		foreach ($temo as $key => $value) {
			echo "<br><b>".$key."</b>: ".$value;
		}
		
		include("inc/parser/parser_xml_diff.php");
		echo "<hr />";
		
		echo "\n\n\n\n\n\n\n\n<pre>\n\n\n\n";
		echo parser_xd("notepad_de.xml", "out", $temo);
		echo "\n\n\n\n</pre>\n\n\n\n\n\n\n\n";

	}
	else {
	
		/*
			// Start parser4:
			$RTRANS_temp_contentb = parser4("notepad_en.xml");
			echo $RTRANS_temp_contentb;
		*/
		
		include("inc/parser/parser_xml_compare.php");
		
		echo '<textarea name="textfield" cols="50" rows="3">'."You didn't enter any text. \\nPlease type something and try again".'</textarea>';
		
		$RTRANS_temp_contentb = parser_xc("notepad_de.xml", "STRING_NOTEXT");
		echo '<textarea name="textfield" cols="50" rows="3">'.$RTRANS_temp_contentb.'</textarea>';
		
		echo "<p>###</p><hr /><hr /><hr /><p>###</p>";
		
		// Start parser3:
		include("inc/parser/parser3.php");
		
		if ( !is_language($ROST_SET_lang) ) {
			$ROST_SET_lang = "en";
		}
		
		if ( !is_translations($ROST_SET_entry) ) {
			$ROST_xml_content = "asdf";
		}
		
		
		$RTRANS_temp_content = parser3("notepad_de.xml", "trans");
		if ($RTRANS_temp_content) {
			echo '<form action="'.$ROST_intern_link_trans_edit.'" method="post">';
			echo $RTRANS_temp_content;
			echo '<p><input name="submit" type="submit" value="Submit"/></p>';
			echo '</form>';
		}
	
	}


?>
