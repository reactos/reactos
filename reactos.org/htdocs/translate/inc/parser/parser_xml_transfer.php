<?php

// Extract XML node from XML file

	$RTRANS_parser_transfer_debug = true;
	
	function parser_transfer ($RTRANS_parser_transfer_rcfile, $RTRANS_parser_transfer_rclang, $RTRANS_parser_transfer_revision, $RTRANS_parser_transfer_id) {  // load single ID
		global $RTRANS_parser_transfer_debug;

		$RTRANS_parser_transfer_content = "";
		
		 
	
		$result_app = SQL_query_array("SELECT * 
										FROM `apps` 
										WHERE `app_name` = '". SQL_escape($RTRANS_parser_transfer_rcfile) ."' 
										AND `app_enabled` = '1'
										LIMIT 1 ;");		
		//echo "<br />app: ". $result_app['app_id'];	

		$result_translation = SQL_query_array("SELECT * 
												FROM `translations`
												WHERE `app_id` = '". SQL_escape($result_app['app_id']) ."'  
												AND `xml_lang` = '". SQL_escape($RTRANS_parser_transfer_rclang) ."'  
												AND `xml_rev` = '". SQL_escape($RTRANS_parser_transfer_revision) ."'  
												LIMIT 1 ;");	
		//echo "<br />content: ". $result_translation['xml_content'];	

		if ($result_translation['xml_content'] != "") {

			$xml = new EFXMLElement( utf8_encode( $result_translation['xml_content'] ) );
			$xml_2 = simplexml_load_string( utf8_encode( $result_translation['xml_content'] ) );
			//var_dump($tst->group[0]->obj[0]->attributes()->type);
			//echo $xml->group[0]->obj[0]->attributes()->type;		
			//echo "<p>&nbsp;</p><hr /><p>&nbsp;</p>";
			

		
			/*$RTRANS_parser_transfer_content .= "->". $xml->group[0]->obj[0];
			
			$RTRANS_parser_transfer_content .= "<p>&nbsp;</p><hr /><p>&nbsp;</p>";*/


			for ($counter_grp = 0; ($xml->group[$counter_grp] != "" && $xml->group[$counter_grp]->obj[0]->attributes()->type != ""); $counter_grp++) {
				$counter_obj = 0;
				foreach ($xml->group[$counter_grp] as $obj_node) {
					//$RTRANS_parser_transfer_content .= "<p><b><u>Unknown-Type:</u></b> ". $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->type ."</p>";
					//$RTRANS_parser_transfer_content .= "<br /># ".$xml->group[$counter_grp]->obj[$counter_obj] ." ### ". $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name;
					if ( $xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name == $RTRANS_parser_transfer_id ) {
						//$RTRANS_parser_transfer_content .= "<p><b>FOUND:</b> ".$xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name." | <i>".$xml->group[$counter_grp]->obj[$counter_obj]."</i>END</p>";
						$RTRANS_parser_transfer_content .= $xml->group[$counter_grp]->obj[$counter_obj];
						$RTRANS_parser_transfer_content .= "]]";
						foreach($xml_2->group[$counter_grp]->obj[$counter_obj]->attributes() as $aa => $bb) {
							$RTRANS_parser_transfer_content .= $aa ."=". $bb ."|";
						}
						//$RTRANS_parser_transfer_content .= "\n(".$xml->group[$counter_grp]->obj[$counter_obj]->attributes()->rc_name.")";
					}
					$counter_obj++;
				}
			}

			return $RTRANS_parser_transfer_content;
		   
		}
		else {
			return 0;
		}
	}

?>