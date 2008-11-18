<?php

$dh = @opendir('.');
if (!$dh) {
    die('Failure');
}

// Only non-UTF languages!
$ext = array(
    'tw'    => 'big5',
    'se'    => 'ISO-8859-1',
    'pt_PT' => 'ISO-8859-1',
    'pt'    => 'ISO-8859-1',
    'no'    => 'ISO-8859-1',
    'nl'    => 'ISO-8859-1',
    'it'    => 'ISO-8859-1',
    'is'    => 'ISO-8859-1',
    'hu'    => 'ISO-8859-2',
    'fr'    => 'ISO-8859-1',
    'es'    => 'ISO-8859-15',
    'en'    => 'ISO-8859-1',
    'de'    => 'ISO-8859-1',
    'da'    => 'ISO-8859-1',
    'cz'    => 'ISO-8859-2',
    'cs'    => 'windows-1250',
    'bg'    => 'windows-1251',
    'zh'    => 'gb2312'
);

$htmlarea_iso = array(
    'da' => 'da-utf',
    'de' => 'de-utf',
    'es' => 'es-utf',
    'fr' => 'fr-utf',
    'it' => 'it-utf',
    'nl' => 'nl-utf',
    'no' => 'no-utf',
    'pt' => 'pt_pt-utf',
    'pt_PT' => 'pt_pt-utf',
    'se' => 'se-utf',
    'cs' => 'cs-utf',
    'cz' => 'cs-utf'
);

$sr = array(
    'bg_BG.CP1251' => 'bg_BG.UTF-8'
);

while (($file = readdir($dh)) !== false) {
    if (!preg_match('@lang_(.+)\.inc\.php$@i', $file, $extmatch)) {
        continue;
    }
    
    if (!isset($ext[$extmatch[1]])) {
        echo "'$file' already is in UTF-8. Leaving untouched.\n";
    } else {
        $set = $ext[$extmatch[1]];
        $cmd = 'iconv -f ' . $set . ' -t UTF-8 -o ' . $file . '.new ' . $file . "\n";
        echo $cmd;
        $return = `$cmd`;
        chmod($file, 0644);
        $fc = file_get_contents($file . '.new');
        $fc = preg_replace('@' . $set . '@i', 'UTF-8', $fc);
        if (isset($htmlarea_iso[$extmatch[1]])) {
            $fc = preg_replace(
                '@define\(\'WYSIWYG_LANG\',\s+\'[^\']+\'\);@i',
                "define('WYSIWYG_LANG', '" . $htmlarea_iso[$extmatch[1]] . "');",
                $fc
            );
        }
        $fc = str_replace(array_keys($sr), array_values($sr), $fc);
        $fp = fopen($file, 'w');
        fwrite($fp, $fc);
        fclose($fp);
        unlink($file . '.new');
    }
}

closedir($dh);