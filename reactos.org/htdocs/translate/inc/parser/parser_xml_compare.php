<?php

// Extract XML node data (only) from a XML file

	$RTRANS_parser_xc_debug = true;
	
	function parser_xc ($RTRANS_parser_xc_rcfile, $RTRANS_parser_xc_rclang, $RTRANS_parser_xc_revision, $RTRANS_parser_xc_id) {
		global $RTRANS_parser_xc_debug;

		$RTRANS_parser_xc_content = "";

		//echo "<h2>APP: ".$RTRANS_parser_xc_rcfile."</h2>";

		$result_app = SQL_query_array("SELECT * 
										FROM `apps` 
										WHERE `app_name` = '". SQL_escape($RTRANS_parser_xc_rcfile) ."' 
										AND `app_enabled` = '1'
										LIMIT 1 ;");		

		$result_translation = SQL_query_array("SELECT * 
												FROM `translations`
												WHERE `app_id` = '". SQL_escape($result_app['app_id']) ."'  
												AND `xml_lang` = '". SQL_escape($RTRANS_parser_xc_rclang) ."'  
												AND `xml_rev` = '". SQL_escape($RTRANS_parser_xc_revision) ."'  
												LIMIT 1 ;");	

		//echo "<p>CONTENT: ".$result_translation['xml_content']."</p>";
		if ($result_translation['xml_content'] != "") {

			$xml = new EFXMLElement( utf8_encode( $result_translation['xml_content'] ) );
			//var_dump($tst->group[0]->obj[0]->attributes()->type);
			//echo $xml->group[0]->obj[0]->attributes()->type;		
			//echo "<p>&nbsp;</p><hr /><p>&nbsp;</p>";
			

		
			/*$RTRANS_parser_xc_content .= "->". $xml->group[0]->obj[0];
			
			$RTRANS_parser_xc_content .= "<p>&nbsp;</p><hr /><p>&nbsp;</p>";*/


			for ($counter_grp = 0; ($xml->group[$counter_grp] != "" && $xml->group[$counter_grp]->obj[0]->attributes()->type != ""); $counter_grp++) {
				$counter_obj = 0;
				foreach ($xml->group[$counter_grp] as $obj_node) {
					//$RTRANS_parser_xc_content .= "<p><b><u>Unknown-Type:</u></b> ". $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->type ."</p>";
					//$RTRANS_parser_xc_content .= "<br /># ".$xml->group[$counter_grp]->obj[$counter_obj] ." ### ". $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name;
					if ( $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name == $RTRANS_parser_xc_id ) {
						//$RTRANS_parser_xc_content .= "<p><b>FOUND:</b> ".$xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name." | <i>".$xml->group[$counter_grp]->obj[$counter_obj]."</i>END</p>";
						$RTRANS_parser_xc_content .= $xml->group[$counter_grp]->obj[$counter_obj];
						//$RTRANS_parser_xc_content .= "\n(".$xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name.")";
					}
					$counter_obj++;
				}
			}

			return $RTRANS_parser_xc_content;
		   
		}
		else {
			return 0;
		}
	}

?>