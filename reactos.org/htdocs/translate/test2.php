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
	
	if (file_exists("notepad_en.xml")) {
		$xmlb = new EFXMLElement(file_get_contents("notepad_en.xml"));
		//var_dump($tst->group[0]->obj[0]->attributes()->type);
		echo "<br>! ".$xmlb->group[0]->obj[0]->attributes()->type;		
		echo "<p>&nbsp;</p><hr /><p>&nbsp;</p>";

		
		echo "<br>! ".$xmlb->group[$counter_grp];
		//echo "<br>! ".$xmlb->group[$counter_grp]->obj[0]->attributes()->type;
		
	}
	else {
		die("???");
	}
	
	echo "<p>&nbsp;</p><hr /><p>&nbsp;</p>";
	echo "<p>&nbsp;</p><hr /><p>&nbsp;</p>";


	$test4 = simplexml2ISOarray(file_get_contents("notepad_en.xml"));

	function simplexml2ISOarray($xml,$attribsAsElements=0) {
	   if (get_class($xml) == 'SimpleXMLElement') {
		   $attributes = $xml->attributes();
		   foreach($attributes as $k=>$v) {
			   if ($v) $a[$k] = (string) $v;
		   }
		   $x = $xml;
		   $xml = get_object_vars($xml);
	   }
	   if (is_array($xml)) {
		   if (count($xml) == 0) return (string) $x; // for CDATA
		   foreach($xml as $key=>$value) {
			   $r[$key] = simplexml2ISOarray($value,$attribsAsElements);
			   if (!is_array($r[$key])) $r[$key] = utf8_decode($r[$key]);
		   }
		   if (isset($a)) {
			   if($attribsAsElements) {
				   $r = array_merge($a,$r);
			   } else {
				   $r['@'] = $a; // Attributes
			   }
		   }
		   return $r;
	   }
	   return (string) $xml;
	} 

	echo "<p>&nbsp;</p><hr /><p>&nbsp;</p>";

	echo "<br>".$test4[0];
	echo "<br>".$test4[1];
	echo "<br>".$test4[2];
	
	echo "<p>&nbsp;</p><hr /><p>&nbsp;</p>";
	print("<pre>");
	print_r($test4);
	print("</pre>"); 
	echo "<p>&nbsp;</p><hr /><p>&nbsp;</p>";


function sxml_to_array($xml) {
   $arr = array();
   $x = 0;
   foreach($xml as $a=>$b) {
       $arr[$a][$x] =  array();
       
       // Looking for ATTRIBUTES
       $att = $b->attributes();        
       foreach($att as $c=>$d) {
           $arr[$a][$x]['@'][$c] = (string) $d;
       }
       
       // Getting CDATA
       $arr[$a][$x]['cdata'] = trim((string) utf8_decode($b));
       
       // Processing CHILD NODES
       $arr[$a][$x]['nodes'] = sxml_to_array($b);
       $x++;
   }

   return $arr;
}

$xml = simplexml_load_file('notepad_en.xml');
$arr = sxml_to_array($xml);
print("<pre>");
print_r($arr);
print("</pre>"); 

?>