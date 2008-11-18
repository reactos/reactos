<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2005  Klemens Friedl <frik85@reactos.org>

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
	
	require("inc/data_menu.php");
	
	global $roscms_intern_account_id;
	global $roscms_standard_language;
	
	$RosCMS_GET_branch = "website";

?>
	<p>&nbsp;</p>
	<h2>Welcome</h2>
	<p><b>Welcome to RosCMS v3</b></p>
	<p>&nbsp;</p>
	<h3>Content</h3>
	<ul>
		<li><b><a href="#web_news">RosCMS Website News</a></b></li>
	</ul>
	<ul>
		<li><b><a href="#web_news_langgroup">Translator Information</a></b></li>
	</ul>
	<p>&nbsp;</p>

	<?php
	
		echo "<a name=\"web_news\"></a><h3>".get_content("web_news", "system", "en", "title", "stext")."</h3>";
		echo "<p><b>".get_content("web_news", "system", "en", "heading", "stext")."</b></p>";
		echo get_content("web_news", "system", "en", "content", "text");
		echo "<p>&nbsp;</p>";
	
		if (roscms_security_grp_member("translator") || roscms_security_grp_member("transmaint")) {
			$query_usrlang = mysql_query("SELECT user_language 
											FROM users 
											WHERE user_id = '".mysql_real_escape_string($roscms_intern_account_id)."' 
											LIMIT 1;");
			$result_usrlang = mysql_fetch_array($query_usrlang);
			
			if ($result_usrlang['user_language'] != "") {
				echo "<a name=\"web_news_langgroup\"></a><h3>Translator Information</h3>";

				$tmp_heading = get_content("web_news_langgroup", "system", $result_usrlang['user_language'], "heading", "stext");
				if ($tmp_heading == "") {
					echo "<p><i>No language group information is available in '".$result_usrlang['user_language']."'-language. The language maintainer should translate and update the 'web_news_langgroup' entry.</i></p>";
					$tmp_heading = get_content("web_news_langgroup", "system", $roscms_standard_language, "heading", "stext");
				}

				$tmp_content = get_content("web_news_langgroup", "system", $result_usrlang['user_language'], "content", "text");
				if ($tmp_content == "") {
					$tmp_content = get_content("web_news_langgroup", "system", $roscms_standard_language, "content", "text");
				}
				echo $tmp_content;
			}
			else {
				echo "<h2>Please set your favorite language in the myReactOS settings.</h2>";
				echo "<p>This language will also be the default language to that you can translate content.</p>";
			}
		}
	?>
	<p>&nbsp;</p>


