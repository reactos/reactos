<?php

// http://webxadmin.free.fr/article/php-recursive-dir-and-sort-by-date-344.php

function LoadFiles($dir,$filter="") {
	$Files = array();
	$It =  opendir($dir);
	if (! $It) {
		die('Cannot list files for ' . $dir);
	}
	
	while ($Filename = readdir($It))
	 {
	 if ($Filename != '.' && $Filename != '..'  )
	 {
	  if(is_dir($dir . $Filename))
	   {
	   $Files = array_merge($Files, LoadFiles($dir . $Filename.'/'));
	   }
	 else 
	  if ($filter=="" || preg_match( "/".$filter."/", $Filename ) ) 
	   {
	   $LastModified = filemtime($dir . $Filename);
	   $Files[] = array($dir .$Filename, $LastModified);
	   }
		
	  else 
	   continue;
	   
	 }
	}
	return $Files;
}
function DateCmp($a, $b) {
	return  strnatcasecmp($a[1] , $b[1]) ;
} 

function SortByDate(&$Files) {
	usort($Files, 'DateCmp');
} 


$Files = LoadFiles("io/", "en.xml");
SortByDate($Files);
reset($Files);

while (list($k,$v) =each($Files)) {
	echo "<li>". $v[0] ." ". date('Y-m-d H:i:s', $v[1]) ."</li>";
} 


?>
