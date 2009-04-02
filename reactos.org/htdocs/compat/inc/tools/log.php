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



	function add_log_entry($level, $category, $laction, $title, $desc, $baduser) {
		global $RSDB_intern_user_id;
	
		$RSDB_referrer="";
		$RSDB_usragent="";
		$RSDB_ipaddr="";
		if (array_key_exists('HTTP_REFERER', $_SERVER)) $RSDB_referrer=htmlspecialchars($_SERVER['HTTP_REFERER']);
		if (array_key_exists('HTTP_USER_AGENT', $_SERVER)) $RSDB_usragent=htmlspecialchars($_SERVER['HTTP_USER_AGENT']);
		if (array_key_exists('REMOTE_ADDR', $_SERVER)) $RSDB_ipaddr=htmlspecialchars($_SERVER['REMOTE_ADDR']);
	
	
    $stmt=CDBConnection::getInstance()->prepare("INSERT INTO rsdb_logs (log_id, log_date, log_usrid, log_usrip, log_level, log_action, log_title, log_description, log_category, log_badusr, log_referrer, log_browseragent, log_read) VALUES ('', NOW( ) , :user_id, :ip, :level, :action, :title, :description, :category, :baduser, :referrer, :user_agent, ';')");
    $stmt->bindParam('user_id',$RSDB_intern_user_id,PDO::PARAM_STR);
    $stmt->bindParam('ip',$RSDB_ipaddr,PDO::PARAM_STR);
    $stmt->bindParam('level',$level,PDO::PARAM_STR);
    $stmt->bindParam('action',$laction,PDO::PARAM_STR);
    $stmt->bindParam('title',$title,PDO::PARAM_STR);
    $stmt->bindParam('description',$desc,PDO::PARAM_STR);
    $stmt->bindParam('category',$category,PDO::PARAM_STR);
    $stmt->bindParam('baduser',$baduser,PDO::PARAM_STR);
    $stmt->bindParam('referrer',$RSDB_referrer,PDO::PARAM_STR);
    $stmt->bindParam('user_agent',$RSDB_usragent,PDO::PARAM_STR);
    $stmt->execute();

	}

?>