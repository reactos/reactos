<?php
$file = fopen ("hw.txt", "a");
if (!$file) {
    echo "<p>Unable to open file for writing.\n";
    exit;
}
fputs ($file, $_POST['data']. "\n");
fclose ($file);
echo "_ok_";
?>