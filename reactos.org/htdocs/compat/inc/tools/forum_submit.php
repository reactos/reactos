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

	$result_page = $stmt->fetchOnce(PDO::FETCH_ASSOC);


//	echo "<h2>".$result_page['comp_name'] ." [". "ReactOS ".show_osversion($result_page['comp_osversion']) ."]</h2>";

//	include("inc/comp/comp_item_menubar.php");

	echo "<h3>Submit a forum post</h3>";

if ($RSDB_intern_user_id <= 0) {
	please_register();
}
else {


	$RSDB_TEMP_SUBMIT_valid = true;

	$RSDB_TEMP_submitpost = "";
	$RSDB_TEMP_txtsubject = "";
	$RSDB_TEMP_txtbody = "";
	$RSDB_TEMP_parententry = "";

	if (array_key_exists("submitpost", $_POST)) $RSDB_TEMP_submitpost=htmlspecialchars($_POST["submitpost"]);
	if (array_key_exists("txtsubject", $_POST)) $RSDB_TEMP_txtsubject=htmlspecialchars($_POST["txtsubject"]);
	if (array_key_exists("txtbody", $_POST)) $RSDB_TEMP_txtbody=htmlspecialchars($_POST["txtbody"]);
	if (array_key_exists("parententry", $_POST)) $RSDB_TEMP_parententry=htmlspecialchars($_POST["parententry"]);


	if ($RSDB_SET_entry != "0") {
		if ($RSDB_TEMP_txtsubject == "") {
      $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp_forum WHERE fmsg_visible = '1' AND fmsg_id = " .  . "");
      $stmt->bindParam('msg_id',$RSDB_SET_entry,PDO::PARAM_STR);
      $stmt->execute();

			$result_page_entry = $stmt->fetchOnce(PDO::FETCH_ASSOC);
			$RSDB_TEMP_txtsubject = "Re: ".$result_page_entry['fmsg_subject'];
		}
	}
	else {
		$RSDB_TEMP_txtsubject = "";
	}

	if ($RSDB_TEMP_submitpost == "yes") {
		if (strlen($RSDB_TEMP_txtsubject) <= 3) {
			msg_bar("The subject textfield is (almost) empty  ...");
			echo "<br />";
			$RSDB_TEMP_SUBMIT_valid = false;
		}
		if (strlen($RSDB_TEMP_txtbody) <= 3) {
			msg_bar("The body textbox is (almost) empty  ...");
			$RSDB_TEMP_SUBMIT_valid = false;
			echo "<br />";
		}
		if (strlen($RSDB_TEMP_parententry) < 1) {
			$RSDB_TEMP_parententry = "0";
		}
	}
	if ($RSDB_TEMP_SUBMIT_valid == "yes" && $RSDB_TEMP_submitpost == true) {
		$rem_adr = "";
		if (array_key_exists('REMOTE_ADDR', $_SERVER)) $rem_adr=htmlspecialchars($_SERVER['REMOTE_ADDR']);

    $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp_forum ORDER BY fmsg_date DESC LIMIT 1");
    $stmt->execute();
		$result_fmsgforum = $stmt->fetchOnce(PDO::FETCH_ASSOC);

		if ($result_fmsgforum['fmsg_body'] != $RSDB_TEMP_txtbody && $RSDB_intern_user_id != 0) {
      $stmt=CDBConnection::getInstance()->prepare("INSERT INTO `rsdb_item_comp_forum` ( `fmsg_id` , `fmsg_comp_id` , `fmsg_parent` , `fmsg_visible` , `fmsg_subject` , `fmsg_body` , `fmsg_user_id` , `fmsg_user_ip` , `fmsg_date` , `fmsg_useful_vote_value` , `fmsg_useful_vote_user` , `fmsg_useful_vote_user_history` )
							VALUES ('', :comp_id, :parent, '1', :subject, :body, :user_id, :ip, NOW( ) , '0', '0', '')");
      $stmt->bindParam('comp_id',$RSDB_SET_item,PDO::PARAM_STR);
      $stmt->bindParam('parent',$RSDB_TEMP_parententry,PDO::PARAM_STR);
      $stmt->bindParam('subject',$RSDB_TEMP_txtsubject,PDO::PARAM_STR);
      $stmt->bindParam('body',$RSDB_TEMP_txtbody,PDO::PARAM_STR);
      $stmt->bindParam('user_id',$RSDB_intern_user_id,PDO::PARAM_STR);
      $stmt->bindParam('ip',$rem_adr,PDO::PARAM_STR);
      $stmt->execute();

			echo "<p><b>Your forum post has been saved!</b></p>";
			include("inc/tools/forum.php");

			// Stats update:
      $stmt=CDBConnection::getInstance()->prepare("UPDATE rsdb_stats SET stat_s_icbb = (stat_s_icbb + 1) WHERE stat_date = :date");
      $stmt->bindValue('date',date("Y-m-d"),PDO::PARAM_STR);
      $stmt->execute();
		}
		else {
			msg_bar("Double post ...");
			echo "<br />";
			include("inc/tools/forum.php");
		}
	}
	else {
?>

<form name="RSDB_forum_post" method="post" action="<?php echo $RSDB_intern_link_submit_forum_post; ?>">
<p><font size="2"><strong>Subject</strong><br />
    <input name="txtsubject" type="text" id="txtsubject" value="<?php echo $RSDB_TEMP_txtsubject; ?>" size="70" maxlength="250">
</font></p>
<p><font size="2"><strong>Body</strong><br />
      <textarea name="txtbody" cols="70" rows="5" id="txtbody"><?php echo $RSDB_TEMP_txtbody; ?></textarea>
</font></p>
<p><font size="2">Please add the <strong>ReactOS version</strong> (and the revision if you use a svn snapshot) and some<strong> hardware information</strong> if you want to post problems, bugs, etc. </font></p>
<p><font size="2">
  <input name="submitpost" type="hidden" id="submitpost" value="yes">
  <input name="parententry" type="hidden" value="<?php
	if ($RSDB_SET_entry != "") {
		echo $RSDB_SET_entry;
	}
  	else {
		echo $RSDB_TEMP_parententry;
	}
  ?>">
  Everyone will be able to rate your forum post.</font></p>
<p><font size="1">The ReactOS administrator and moderator team of the ReactOS homepage will attempt to remove or edit any generally objectionable material as quickly as possible, it is impossible to review every message/comment/entry/etc. Therefore you acknowledge that all posts/comment/entry/etc. made to the ReactOS homepage express the views and opinions of the author and not the administrators, moderators or webmaster (except for posts/comment/entry/etc. by these people) and hence will not be held liable. </font></p>
<p><font size="1">You agree not to post any abusive, obscene, vulgar, slanderous, hateful, threatening, sexually-oriented or any other material that may violate any applicable laws. Doing so may lead to you being immediately and permanently banned (and your service provider being informed). The IP address of all posts is recorded to aid in enforcing these conditions. You agree that the webmaster, administrator and moderator team of the ReactOS homepage have the right to remove, edit, move or close any topic/entry/etc. at any time should they see fit. As a user you agree to any information you have entered above being stored in a database. While this information will not be disclosed to any third party without your consent the webmaster, administrator and moderator team cannot be held responsible for any hacking attempt that may lead to the data being compromised. </font></p>
<p> <font size="2">By clicking &quot;Submit&quot; below you agree to be bound by these conditions </font></p>
<p>
  <input type="submit" name="Submit" value="Submit">
</p>
</form>
<?php
		}
	}
?>
<p>&nbsp;</p>
