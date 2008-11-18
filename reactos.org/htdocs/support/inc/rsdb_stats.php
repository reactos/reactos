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



			$check_if_date_exist="SELECT COUNT('stat_date') FROM rsdb_stats WHERE stat_date ='". date("Y-m-d") ."'";
			$check_if_date_exist_query=mysql_query($check_if_date_exist);	
			$check_if_date_exist_result = mysql_fetch_row($check_if_date_exist_query);
			if ($check_if_date_exist_result[0] == 0) {
				// Insert a new date entry
				$insert_new_date="INSERT INTO `rsdb_stats` ( `stat_date` , `stat_pviews` , `stat_visitors` , `stat_users` , `stat_s_cat` , `stat_s_grp` , `stat_s_icomp` , `stat_s_ictest` , `stat_s_icbb` , `stat_s_icvotes` , `stat_s_media` , `stat_s_votes` , `stat_vislst` , `stat_usrlst` , `stat_brow_IE` , `stat_brow_MOZ` , `stat_brow_OPERA` , `stat_brow_KHTML` , `stat_brow_text` , `stat_brow_other` , `stat_os_winnt` , `stat_os_ros` , `stat_os_unix` , `stat_os_bsd` , `stat_os_linux` , `stat_os_mac` , `stat_os_other` , `stat_reflst` ) 
									VALUES (
									CURDATE( ) , '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '|', '|', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '|');";
				$insert_new_date_submit=mysql_query($insert_new_date);
				
				// Remove the user data (ip-address, browser-agent data, etc.) from the "two days before yesterday"-entry (user data is saved for three days):
				$query_date_entry_records=mysql_query("SELECT stat_date, stat_pviews
														FROM `rsdb_stats`
														WHERE 1 
														ORDER BY `stat_date` DESC 
														LIMIT 3, 3 ;");	
				$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
				
				$update_date_entrya = "UPDATE `rsdb_stats` SET 
										`stat_vislst` = '', 
										`stat_usrlst` = '', 
										`stat_reflst` = '' 
										WHERE `stat_date` = '". $result_date_entry_records['stat_date'] ."' ;";
				mysql_query($update_date_entrya);
			}
			
			$RSDB_stat_visitors="";
			$RSDB_stat_users="";
			$RSDB_stat_vislst="";
			$RSDB_stat_usrlst="";
			$RSDB_stat_browser="";
			$RSDB_stat_os="";
			$RSDB_stat_reflst="";
			
			$RSDB_referrer="";
			$RSDB_usragent="";
			$RSDB_ipaddr="";
			if (array_key_exists('HTTP_REFERER', $_SERVER)) $RSDB_referrer=htmlspecialchars($_SERVER['HTTP_REFERER']);
			if (array_key_exists('HTTP_USER_AGENT', $_SERVER)) $RSDB_usragent=htmlspecialchars($_SERVER['HTTP_USER_AGENT']);
			if (array_key_exists('REMOTE_ADDR', $_SERVER)) $RSDB_ipaddr=htmlspecialchars($_SERVER['REMOTE_ADDR']);
			
			$query_date_entry=mysql_query("SELECT stat_date, stat_visitors, stat_users, stat_vislst, stat_usrlst, stat_reflst
									FROM `rsdb_stats`
									WHERE `stat_date` = '". date("Y-m-d") ."' ;");	
			$result_date_entry = mysql_fetch_array($query_date_entry);
			
	
			$RSDB_TEMP_stat_vislst = strchr($result_date_entry['stat_vislst'],("|".$RSDB_ipaddr."-".$RSDB_usragent."|"));
			$RSDB_TEMP_stat_usrlst = strchr($result_date_entry['stat_usrlst'],("|".$RSDB_intern_user_id."|"));
			$RSDB_TEMP_stat_reflst = strchr($result_date_entry['stat_reflst'],("|".$RSDB_referrer."|"));
			
			if ($RSDB_TEMP_stat_vislst == false) {
				$RSDB_stat_vislst = ", `stat_vislst` = '". $result_date_entry['stat_vislst']. mysql_escape_string($RSDB_ipaddr) ."-". mysql_escape_string($RSDB_usragent) ."|'";
				$RSDB_stat_visitors = ", `stat_visitors` = (stat_visitors + 1)";
			}
			if ($RSDB_TEMP_stat_usrlst == false) {
				$RSDB_stat_usrlst = ", `stat_usrlst` = '". $result_date_entry['stat_usrlst']. mysql_escape_string($RSDB_intern_user_id) ."|'";
				$RSDB_stat_users = ", `stat_users` = (stat_users + 1)";
			}
			if ($RSDB_TEMP_stat_reflst == false) {
				if (!preg_match("=http\:\/\/www\.reactos.com\/=", $RSDB_referrer) && !preg_match("=http\:\/\/www\.reactos.org\/=", $RSDB_referrer)) {
					$RSDB_stat_reflst = ", `stat_reflst` = '". $result_date_entry['stat_reflst']. mysql_escape_string($RSDB_referrer) ."|'";
				}
			}
			
			
			$brows="";
			if ($RSDB_TEMP_stat_vislst == false) {
				// Browser
				if (preg_match("=MSIE [0-9]{1,2}.[0-9]{1,2}.*Opera.([0-9]{1})=", $RSDB_usragent, $brows)) {
					$RSDB_stat_browser = ", `stat_brow_OPERA` = (stat_brow_OPERA + 1)";
				}
				elseif (preg_match("=MSIE=", $RSDB_usragent)) {
					$RSDB_stat_browser = ", `stat_brow_IE` = (stat_brow_IE + 1)";
				}
				elseif (preg_match("=Opera=", $RSDB_usragent)) {
					$RSDB_stat_browser = ", `stat_brow_OPERA` = (stat_brow_OPERA + 1)";
				}
				elseif (preg_match("=Mosaic=", $RSDB_usragent)) {
					$RSDB_stat_browser = ", `stat_brow_other` = (stat_brow_other + 1)";
				}
				elseif (preg_match("=OB1=", $RSDB_usragent)) {
					$RSDB_stat_browser = ", `stat_brow_other` = (stat_brow_other + 1)";
				}
				elseif (preg_match("=Safari=", $RSDB_usragent)) {
					$RSDB_stat_browser = ", `stat_brow_KHTML` = (stat_brow_KHTML + 1)";
				}
				elseif (preg_match("=Dillo=", $RSDB_usragent)) {
					$RSDB_stat_browser = ", `stat_brow_other` = (stat_brow_other + 1)";
				}
				elseif (preg_match("=Lynx=", $RSDB_usragent)) {
					$RSDB_stat_browser = ", `stat_brow_text` = (stat_brow_text + 1)";
				}
				elseif (preg_match("=Konqueror=", $RSDB_usragent)) {
					$RSDB_stat_browser = ", `stat_brow_KHTML` = (stat_brow_KHTML + 1)";
				}
				elseif (preg_match("=Netscape=", $RSDB_usragent)) {
					$RSDB_stat_browser = ", `stat_brow_MOZ` = (stat_brow_MOZ + 1)";
				}
				elseif (preg_match("=Mozilla=", $RSDB_usragent)) {
					$RSDB_stat_browser = ", `stat_brow_MOZ` = (stat_brow_MOZ + 1)";
				}
		
				// Operating System
				if(preg_match("=Windows|Win=", $RSDB_usragent)) {
					$RSDB_stat_os = ", `stat_os_winnt` = (stat_os_winnt + 1)";
				}
				elseif (preg_match("=ReactOS=", $RSDB_usragent)) {
					$RSDB_stat_os = ", `stat_os_ros` = (stat_os_ros + 1)";
				}
				elseif (preg_match("=Linux=", $RSDB_usragent)) {
					$RSDB_stat_os = ", `stat_os_linux` = (stat_os_linux + 1)";
				}
				elseif (preg_match("=Unix|SunOS|Solaris=", $RSDB_usragent)) {
					$RSDB_stat_os = ", `stat_os_unix` = (stat_os_unix + 1)";
				}
				elseif (preg_match("=BSD=", $RSDB_usragent)) {
					$RSDB_stat_os = ", `stat_os_bsd` = (stat_os_bsd + 1)";
				}
				elseif (preg_match("=Darwin|Mac OS X|Mac=", $RSDB_usragent)) {
					$RSDB_stat_os = ", `stat_os_mac` = (stat_os_mac + 1)";
				}
				elseif (preg_match("=VMS|OpenVMS=", $RSDB_usragent)) {
					$RSDB_stat_os = ", `stat_os_other` = (stat_os_other + 1)";
				}
				elseif (preg_match("=OS\/2|OS2=", $RSDB_usragent)) {
					$RSDB_stat_os = ", `stat_os_other` = (stat_os_other + 1)";
				}
				elseif (preg_match("=BeOS|Haiku|SkyOS=", $RSDB_usragent)) {
					$RSDB_stat_os = ", `stat_os_other` = (stat_os_other + 1)";
				}
				elseif (preg_match("=DOS=", $RSDB_usragent)) {
					$RSDB_stat_os = ", `stat_os_other` = (stat_os_other + 1)";
				}
			}
			
			
			/*
				UPDATE `rsdb_stats` SET
				`stat_pviews` = '2',
				`stat_visitors` = '1',
				`stat_users` = '1',
				`stat_vislst` = ',1,',
				`stat_usrlst` = ',1,',
				`stat_brow_IE` = '1',
				`stat_os_winnt` = '1',
				`stat_reflst` = '???',
				WHERE `stat_date` = '2006-04-14' LIMIT 1 ;
			*/
			
			$update_date_entry = "UPDATE `rsdb_stats` SET
									`stat_pviews` = (stat_pviews + 1) 
									". $RSDB_stat_vislst ." 
									". $RSDB_stat_visitors ." 
									". $RSDB_stat_usrlst ." 
									". $RSDB_stat_users ." 
									". $RSDB_stat_reflst ." 
									". $RSDB_stat_browser ." 
									". $RSDB_stat_os ." 
									WHERE `stat_date` = '". date("Y-m-d") ."' LIMIT 1 ;";
			mysql_query($update_date_entry);
/*
				$insert_new_date="INSERT INTO `rsdb_stats` ( `stat_date` , `stat_pviews` , `stat_visitors` , `stat_users` , `stat_s_cat` , `stat_s_grp` , `stat_s_icomp` , `stat_s_ictest` , `stat_s_icbb` , `stat_s_icvotes` , `stat_s_media` , `stat_s_votes` , `stat_usrlst` , `stat_brow_IE` , `stat_brow_MOZ` , `stat_brow_OPERA` , `stat_brow_KHTML` , `stat_brow_text` , `stat_brow_other` , `stat_os_winnt` , `stat_os_windos` , `stat_os_unix` , `stat_os_bsd` , `stat_os_linux` , `stat_os_mac` , `stat_reflst` ) 
								VALUES ('', '".mysql_real_escape_string($RSDB_SET_item)."', '1', '".mysql_real_escape_string($RSDB_TEMP_txtwhatwork)."', '".mysql_real_escape_string($RSDB_TEMP_txtwhatnot)."', '".mysql_real_escape_string($RSDB_TEMP_txtwhatnottested)."', NOW( ), '".mysql_real_escape_string($RSDB_TEMP_optinstall)."', '".mysql_real_escape_string($RSDB_TEMP_optfunc)."', '".mysql_real_escape_string($RSDB_TEMP_txtcomment)."', '".mysql_real_escape_string($RSDB_TEMP_txtconclusion)."', '".mysql_real_escape_string($RSDB_intern_user_id)."', NOW( ) , '', '', '', '".mysql_real_escape_string($RSDB_TEMP_txtrevision)."' );";
*/

?>