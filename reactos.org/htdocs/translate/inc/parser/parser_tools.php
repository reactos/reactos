<?php

	class EFXMLElement extends SimpleXMLElement {
	
	   function attributes() {
		   return new EFXMLAttributes(parent::attributes());
	   }
	
	}
	
	class EFXMLAttributes {
	
	   private $attributes;
	
	   function __construct($attributes) {
		   $this->attributes = $attributes;
	   }
	
	   function __get($name) {
		   if (isset($this->attributes[$name])) {
			   return $this->attributes[$name];
		   } else {
			   return false;
		   }
	   }
	
	}
	
	
	$ROST_color_counter = 0;
	
	function create_textfield_translate_readonly ($ROST_textfield_value1, $ROST_textfield_value2) {
		$ROST_textfield_content = '<textarea name="';
		$ROST_textfield_content .= $ROST_textfield_value1;
		$ROST_textfield_content .= '" cols="35" rows="3" readonly="true">';
		$ROST_textfield_content .= $ROST_textfield_value2;
		$ROST_textfield_content .= '</textarea>';
		return $ROST_textfield_content;
	}
	
	function create_textfield_translate ($ROST_textfield_value1, $ROST_textfield_value2) {
		$ROST_textfield_content = '<textarea name="';
		$ROST_textfield_content .= $ROST_textfield_value1;
		$ROST_textfield_content .= '" cols="35" rows="3">';
		$ROST_textfield_content .= $ROST_textfield_value2;
		$ROST_textfield_content .= '</textarea>';
		return $ROST_textfield_content;
	}
	
	
	function create_table_header_translate () {
		$ROST_table_content = '<table width="100%" border="0" cellpadding="1" cellspacing="1">';
		$ROST_table_content .= '<tr bgcolor="#5984C3">';
			$ROST_table_content .= '<td width="35%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>ID</strong></font></div></td>';
			$ROST_table_content .= '<td width="25%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>English</strong></font></div></td>';
			$ROST_table_content .= '<td width="25%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>German</strong></font></div></td>';
			$ROST_table_content .= '<td width="15%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Options</strong></font></div></td>';
		$ROST_table_content .= '</tr>	';
		
		return $ROST_table_content;
	}

	function reset_color_counter () {
		global $ROST_color_counter;
		
		$ROST_color_counter = 0;
	}

	function create_table_body_translate ($ROST_table_content1, $ROST_table_content2, $ROST_table_content3) {
		global $ROST_color_counter;
		
		$ROST_color1="#E2E2E2";
		$ROST_color2="#EEEEEE";
		$ROST_color_current = "";
		
	
		$ROST_color_counter++;
		if ($ROST_color_counter == "1") {
			$ROST_color_current = $ROST_color1;
			$ROST_color = $ROST_color1;
		}
		elseif ($ROST_color_counter == "2") {
			$ROST_color_counter="0";
			$ROST_color_current = $ROST_color2;
			$ROST_color = $ROST_color2;
		}
		
		$ROST_table_content = '<tr bgcolor="'.$ROST_color_current.'">';
			$ROST_table_content .= '<td bgcolor="'.$ROST_color_current.'"><div align="left"><font size="2" face="Verdana, Arial, Helvetica, sans-serif"><b>'. $ROST_table_content1 .'</b></font></div></td>';
			$ROST_table_content .= '<td bgcolor="'.$ROST_color_current.'"><div align="center"><font size="2" face="Verdana, Arial, Helvetica, sans-serif">'. $ROST_table_content2 .'</font></div></td>';
			$ROST_table_content .= '<td bgcolor="'.$ROST_color_current.'"><div align="center"><font size="2" face="Verdana, Arial, Helvetica, sans-serif">'. $ROST_table_content3 .'</font></div></td>';
			$ROST_table_content .= '<td bgcolor="'.$ROST_color_current.'"><div align="center"><font size="2" face="Verdana, Arial, Helvetica, sans-serif"><input name="chname" type="checkbox" value="true" checked />Test</font></div></td>';
		$ROST_table_content .= '</tr>	';
		
		return $ROST_table_content;
	}
	
	function create_table_footer_translate () {
		$ROST_table_content = '</table>';
		
		return $ROST_table_content;
	}

	// Parse the data from parser_transfer() and put together a xml node
	function write_xml_node ($ROST_xml_node_data, $ROST_debug_mode, $ROST_xml_node_data_addition = "") {
		$ROST_xml_node_content = "";

		$ROST_temp_splitup_content_a = split("[]]]", $ROST_xml_node_data);
		
//		echo "<p>". $ROST_temp_splitup_content_a[0] ."</p>";
		
		$ROST_xml_node_content .= "\n  <obj";
		
		if (@$ROST_temp_splitup_content_a[1] != "") {
			$ROST_temp_splitup_content_temp = $ROST_temp_splitup_content_a[1];
		}
		else {
			$ROST_temp_splitup_content_temp = $ROST_temp_splitup_content_a[0];
		}
		$ROST_temp_splitup_content_b = split("[|]", $ROST_temp_splitup_content_temp);
		
		foreach($ROST_temp_splitup_content_b as $cc) {
			$ROST_temp_splitup_content_c = split("[=]", $cc);
			if (@$ROST_temp_splitup_content_c[1] != "") {
//				echo "<p>". $ROST_temp_splitup_content_c[0] ." =&gt; ". $ROST_temp_splitup_content_c[1] ."</p>";
				$ROST_xml_node_content .= " ". $ROST_temp_splitup_content_c[0] ."=\"". $ROST_temp_splitup_content_c[1] ."\"";
			}
		}
		
		if ($ROST_xml_node_data_addition != "") {
				$ROST_xml_node_content .= " ". $ROST_xml_node_data_addition;
		}
		
		$ROST_xml_node_content .= "><![CDATA[";
		$ROST_xml_node_content .= $ROST_temp_splitup_content_a[0];
		$ROST_xml_node_content .= "]]></obj>";
		
		return $ROST_xml_node_content;
	}




	$RTRANS_parser4_debug = true;
	
	function parser4 ($RTRANS_parser4_rcfile) {
		global $RTRANS_parser4_debug;
	echo $RTRANS_parser4_rcfile;
		$RTRANS_parser4_content = "";

		if (file_exists($RTRANS_parser_4_rcfile)) {

			$xml = new EFXMLElement(file_get_contents($RTRANS_parser3_rcfile));
			//var_dump($tst->group[0]->obj[0]->attributes()->type);
			//echo $xml->group[0]->obj[0]->attributes()->type;		
			//echo "<p>&nbsp;</p><hr /><p>&nbsp;</p>";
			
			$RTRANS_parser4_content .= "<p>&nbsp;</p><hr /><p>&nbsp;</p>";
			
			$RTRANS_parser4_content .= "->". $xml->group[0]->obj[0];
			
			$RTRANS_parser4_content .= "<p>&nbsp;</p><hr /><p>&nbsp;</p>";

		   return $RTRANS_parser4_content;
		   
		}
		else {
			return 0;
		}
	}

?>