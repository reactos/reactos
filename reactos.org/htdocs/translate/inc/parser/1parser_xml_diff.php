<?php

	$RTRANS_parser_diff_debug = true;
	
	function parser_diff ($RTRANS_parser_xc_rcfile, $RTRANS_parser_xc_id) {  // load single ID
		global $RTRANS_parser_xc_debug;

		$RTRANS_parser_xc_content = "";

		if (file_exists($RTRANS_parser_xc_rcfile)) {

			$xml = new EFXMLElement( utf8_encode( file_get_contents($RTRANS_parser_xc_rcfile) ) );
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