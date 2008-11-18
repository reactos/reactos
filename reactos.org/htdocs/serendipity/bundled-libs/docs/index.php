<?php

error_reporting(E_ALL);
require_once 'Text/Wiki.php';


/**
* 
* Gets microtime; for timing how long it takes to process code.
* 
*/

function getmicrotime()
{ 
    list($usec, $sec) = explode(" ",microtime()); 
    return ((float)$usec + (float)$sec); 
}

// Set up the wiki options
$options = array();
$options['view_url'] = "index.php?page=";


// Get the list of existing wiki pages, based on the .wiki.txt
// files in the current directory.
$options['pages'] = array();

$dir = opendir(dirname(__FILE__));

while ($file = readdir($dir)) {
    if (substr($file, -9) == '.wiki.txt') {
        $options['pages'][] = substr($file, 0, -9);
    }
}

closedir($dir);

// what page is being requested?
if (isset($_GET['page'])) {
    $page = $_GET['page'];
} else {
    $page = 'HomePage';
}

// load the text for the requested page
$text = implode('', file($page . '.wiki.txt'));

// create a Wiki objext with the loaded options
$wiki =& new Text_Wiki($options);

// time the operation, and transform the wiki text.
$before = getMicroTime();
$output = $wiki->transform($text);
$after = getMicroTime();
$time = (float)$after - (float)$before;


// output the page!
?>
<?xml version="1.0" encoding="iso-8859-1" ?>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
        "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
        
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">

    <head>
        <meta http-equiv="content-type" content="text/html; charset=iso-8859-1" />
        <title>Text_Wiki::<?php echo $page ?></title>
        <link rel="stylesheet" href="stylesheet.css" type="text/css" />
    </head>
    
    <body>
        <?php echo $output ?>
        <?php echo "<hr /><p>Transformed in $time seconds.</p>" ?>
    </body>
</html>
