<?php 
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2005  Ge van Geldorp <gvg@reactos.org>

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
set_magic_quotes_runtime(0);

$roscms_intern_editor_content="";

if(isset($_POST['textarea_content']) && $_POST['textarea_content'] != "") {
	if (array_key_exists("textarea_content", $_POST)) $roscms_intern_editor_content=$_POST['textarea_content'];
	echo "<p><b>Converted</b></p>";
	echo $roscms_intern_editor_content;
}

/*$roscms_intern_editor_content='</div></td>

  <td id=\"rightNav\">
   <img src=\"[#inc_path_homepage_media_pictures]2005/thumb_ooo114calc.jpg\" alt=\"ReactOS Screenshot\" width=\"266\" height=\"200\" /><br />
      <a href=\"#\">More screenshots</a>
<br />
<h1>Latest Release</h1>
<p><strong>Version [#inc_reactos_version]</strong><br />
    <a href=\"http://www.reactos.org/en/content/view/full/62\">Download Now!</a><br />
    <a href=\"http://www.reactos.org/wiki/index.php/ChangeLog-0.2.6\">Change Log</a>
</p>
[#inc_template_news_latest]
   <div class=\"contentSmall\">
    <span class=\"contentSmallTitle\">Developer Quotes</span>
     \"This is could be some kinda quote\"<br>
     -Dev';*/

?>
<form name="cms_content" method="post" action="convert.php">  <p> 
    <textarea name="textarea_content" cols="60" rows="15" id="textarea_content"><?php 
				echo stripslashes(stripslashes($roscms_intern_editor_content));
				
			?></textarea>
  </p>
  <p>
    <input type="submit" name="Submit" value="Submit">
    | <a href="convert.php">Restart</a></p>
</form>
