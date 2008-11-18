<?php # $Id: functions_images.inc.php 656 2005-11-07 15:28:47Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

function serendipity_isActiveFile($file) {
    if (preg_match('@^\.@', $file)) {
        return true;
    }

    return preg_match('@\.(php[345]?|[psj]html?|aspx?|cgi|jsp|py|pl)$@i', $file);
}

/**
* Get a list of images
**/
function serendipity_fetchImagesFromDatabase($start=0, $limit=0, &$total, $order = false, $ordermode = false, $directory = '', $filename = '') {
    global $serendipity;

    $orderfields = serendipity_getImageFields();
    if (empty($order) || !isset($orderfields[$order])) {
        $order = 'date';
    }

    if (empty($ordermode) || ($ordermode != 'DESC' && $ordermode != 'ASC')) {
        $ordermode = 'DESC';
    }

    if ($limit != 0) {
        $limitsql = serendipity_db_limit_sql(serendipity_db_limit($start, $limit));
    }

    if (!empty($directory)) {
        $directorysql = ' WHERE path LIKE \'' . serendipity_db_escape_string($directory) . '%\' ';
    }

    if (!empty($filename)) {
        if (empty($directorysql)) {
            $directorysql = " WHERE  name like '%" . serendipity_db_escape_string($filename) . "%'";
        } else {
            $directorysql .= " AND   name like '%" . serendipity_db_escape_string($filename) . "%'";
        }
    }
    
    $perm = $permsql = '';
    if (isset($serendipity['authorid']) && !serendipity_checkPermission('adminImagesViewOthers')) {
        $perm = " (i.authorid = 0 OR i.authorid = " . (int)$serendipity['authorid'] . ")";
        if (empty($directorysql)) {
            $directorysql = " WHERE  $perm";
        } else {
            $directorysql .= " AND   $perm";
        }
        $permsql = " WHERE $perm"; 
    }    

    $query = "SELECT i.*, a.realname AS authorname FROM {$serendipity['dbPrefix']}images AS i LEFT OUTER JOIN {$serendipity['dbPrefix']}authors AS a ON i.authorid = a.authorid $directorysql ORDER BY $order $ordermode $limitsql";
    $rs = serendipity_db_query($query, false, 'assoc');
    if (!is_array($rs)) {
        return array();
    }

    $total_query = "SELECT count(i.id) FROM {$serendipity['dbPrefix']}images AS i LEFT OUTER JOIN {$serendipity['dbPrefix']}authors AS a on i.authorid = a.authorid $permsql";
    $total_rs = serendipity_db_query($total_query, true, 'num');
    if (is_array($total_rs)) {
        $total = $total_rs[0];
    }

    return $rs;
}

function serendipity_fetchImageFromDatabase($id) {
    global $serendipity;
    $rs = serendipity_db_query("SELECT * FROM {$serendipity['dbPrefix']}images WHERE id = ". (int)$id, true, 'assoc');
    return $rs;
}

function serendipity_updateImageInDatabase($updates, $id) {
    global $serendipity;

    $admin = '';
    if (!serendipity_checkPermission('adminImagesAdd')) {
        $admin = ' AND (authorid = ' . $serendipity['authorid'] . ' OR authorid = 0)';
    }

    $i=0;
    if (sizeof($updates) > 0) {
        foreach ($updates as $k => $v) {
            $q[] = $k ." = '" . serendipity_db_escape_string($v) . "'";
        }
        serendipity_db_query("UPDATE {$serendipity['dbPrefix']}images SET ". implode($q, ',') ." WHERE id = " . (int)$id . " $admin");
        $i++;
    }
    return $i;
}

function serendipity_deleteImage($id) {
    global $serendipity;
    $file   = serendipity_fetchImageFromDatabase($id);
    $dFile  = $file['path'] . $file['name'] . '.' . $file['extension'];
    $dThumb = $file['path'] . $file['name'] . (!empty($file['thumbnail_name']) ? '.' . $file['thumbnail_name'] : '') . '.' . $file['extension'];

    $admin = '';
    if (!serendipity_checkPermission('adminImagesDelete')) {
        return;
    }

    if (!serendipity_checkPermission('adminImagesMaintainOthers') && $file['authorid'] != '0' && $file['authorid'] != $serendipity['authorid']) {
        // A non-admin user may not delete private files from other users.
        return;
    }

    if (!$file['hotlink']) {
        if (file_exists($serendipity['serendipityPath'] . $serendipity['uploadPath'] . $dFile)) {
            if (@unlink($serendipity['serendipityPath'] . $serendipity['uploadPath'] . $dFile)) {
                printf(DELETE_FILE . '<br />', $dFile);
            } else {
                printf(DELETE_FILE_FAIL . '<br />', $dFile);
            }

            if (@unlink($serendipity['serendipityPath'] . $serendipity['uploadPath'] . $dThumb)) {
                printf(DELETE_THUMBNAIL . '<br />', $dThumb);
            }
        } else {
            printf(FILE_NOT_FOUND . '<br />', $dFile);
        }
    } else {
        printf(DELETE_HOTLINK_FILE . '<br />', $file['name']);
    }

    serendipity_db_query("DELETE FROM {$serendipity['dbPrefix']}images WHERE id = ". (int)$id);

}

/**
* Get a list of images
**/
function serendipity_fetchImages($group = false, $start = 0, $end = 20, $images = '', $odir = '') {
    global $serendipity;

    // Open directory
    $basedir = $serendipity['serendipityPath'] . $serendipity['uploadPath'];
    $images = array();
    if ($dir = @opendir($basedir . $odir)) {
        while(false !== ($f = readdir($dir))) {
            if ($f != '.' && $f != '..' && strpos($f, $serendipity['thumbSuffix']) === false) {
                $cdir = ($odir != '' ? $odir . '/' : '');
                if (is_dir($basedir . $odir . '/' . $f)) {
                    $temp = serendipity_fetchImages($group, $start, $end, $images, $cdir . $f);
                    foreach($temp AS $tkey => $tval) {
                        array_push($images, $tval);
                    }
                } else {
                    array_push($images, $cdir . $f);
                }
            }
        }
    }
    natsort($images);

    /* BC */
    $serendipity['imageList'] = $images;
    return $images;
}

function serendipity_insertHotlinkedImageInDatabase($filename, $url, $authorid = 0, $time = NULL, $tempfile = NULL) {
    global $serendipity;

    if (is_null($time)) {
        $time = time();
    }

    list($filebase, $extension) = serendipity_parseFileName($filename);

    if ($tempfile && file_exists($tempfile)) {
        $filesize = @filesize($tempfile);
        $fdim     = @serendipity_getimagesize($tempfile, '', $extension);
        $width    = $fdim[0];
        $height   = $fdim[1];
        $mime     = $fdim['mime'];
        @unlink($tempfile);
    }

    $query = sprintf(
      "INSERT INTO {$serendipity['dbPrefix']}images (
                    name,
                    date,
                    authorid,
                    extension,
                    mime,
                    size,
                    dimensions_width,
                    dimensions_height,
                    path,
                    hotlink
                   ) VALUES (
                    '%s',
                    %s,
                    %s,
                    '%s',
                    '%s',
                    %s,
                    %s,
                    %s,
                    '%s',
                    1
                   )",
      serendipity_db_escape_string($filebase),
      (int)$time,
      (int)$authorid,
      serendipity_db_escape_string($extension),
      serendipity_db_escape_string($mime),
      (int)$filesize,
      (int)$width,
      (int)$height,
      serendipity_db_escape_string($url)
    );

    $sql = serendipity_db_query($query);
    if (is_string($sql)) {
        echo $query . '<br />';
        echo $sql . '<br />';
    }

    $image_id = serendipity_db_insert_id('images', 'id');
    if ($image_id > 0) {
        return $image_id;
    }

    return 0;
}

function serendipity_insertImageInDatabase($filename, $directory, $authorid = 0, $time = NULL) {
    global $serendipity;

    if ( is_null($time) ) {
        $time = time();
    }

    $filepath = $serendipity['serendipityPath'] . $serendipity['uploadPath'] . $directory . $filename;
    $filesize = @filesize($filepath);

    list($filebase, $extension) = serendipity_parseFileName($filename);

    $thumbpath = $serendipity['serendipityPath'] . $serendipity['uploadPath'] . $directory . $filebase . '.'. $serendipity['thumbSuffix'] . '.'. $extension;
    $thumbnail = (file_exists($thumbpath) ? $serendipity['thumbSuffix'] : '');

    $fdim = @serendipity_getimagesize($filepath, '', $extension);
    $width = $fdim[0];
    $height = $fdim[1];
    $mime = $fdim['mime'];


    $query = sprintf(
      "INSERT INTO {$serendipity['dbPrefix']}images (
                    name,
                    extension,
                    mime,
                    size,
                    dimensions_width,
                    dimensions_height,
                    thumbnail_name,
                    date,
                    authorid,
                    path
                   ) VALUES (
                    '%s',
                    '%s',
                    '%s',
                    %s,
                    %s,
                    %s,
                    '%s',
                    %s,
                    %s,
                    '%s'
                   )",
      serendipity_db_escape_string($filebase),
      serendipity_db_escape_string($extension),
      serendipity_db_escape_string($mime),
      (int)$filesize,
      (int)$width,
      (int)$height,
      serendipity_db_escape_string($thumbnail),
      (int)$time,
      (int)$authorid,
      serendipity_db_escape_string($directory)
    );

    $sql = serendipity_db_query($query);
    if (is_string($sql)) {
        echo $query . '<br />';
        echo $sql . '<br />';
    }

    $image_id = serendipity_db_insert_id('images', 'id');
    if ($image_id > 0) {
        return $image_id;
    }

    return 0;
}


/**
* Generate a thumbnail
**/
function serendipity_makeThumbnail($file, $directory = '', $size = false) {
    global $serendipity;

    if ($size === false) {
        $size = $serendipity['thumbSize'];
    }

    $t       = serendipity_parseFileName($file);
    $f       = $t[0];
    $suf     = $t[1];


    $infile  = $serendipity['serendipityPath'] . $serendipity['uploadPath'] . $directory . $file;
    $outfile = $serendipity['serendipityPath'] . $serendipity['uploadPath'] . $directory . $f . '.' . $serendipity['thumbSuffix'] . '.' . $suf;

    $fdim    = @serendipity_getimagesize($infile, '', $suf);
    if (isset($fdim['noimage'])) {
        $r = array(0, 0);
    } else {
        if ($serendipity['magick'] !== true) {
            $r = serendipity_resize_image_gd($infile, $outfile, $size);
        } else {
            $r = array($size, $size);
            $newSize = $size . 'x' . $size;
            if ($fdim['mime'] == 'application/pdf') {
               $cmd     = escapeshellcmd($serendipity['convert']) . ' -antialias -flatten -scale '. serendipity_escapeshellarg($newSize) .' '. serendipity_escapeshellarg($infile) .' '. serendipity_escapeshellarg($outfile . '.png');
            } else {
               $newSize .= '>'; // Tell imagemagick to not enlarge small images
               $cmd     = escapeshellcmd($serendipity['convert']) . ' -antialias -resize '. serendipity_escapeshellarg($newSize) .' '. serendipity_escapeshellarg($infile) .' '. serendipity_escapeshellarg($outfile);
            }
            exec($cmd, $output, $result);
            if ( $result != 0 ) {
                echo '<div class="serendipityAdminMsgError">'. sprintf(IMAGICK_EXEC_ERROR, $cmd, $output[0], $result) .'</div>';
                $r = false; // return failure
            } else {
               touch($outfile);
            }
            unset($output, $result);
        }
    }

    return $r;
}

/**
*  Scale an image (ignoring proportions)
**/
function serendipity_scaleImg($id, $width, $height) {
    global $serendipity;

    $file = serendipity_fetchImageFromDatabase($id);

    $admin = '';
    if (!serendipity_checkPermission('adminImagesMaintainOthers') && $file['authorid'] != '0' && $file['authorid'] != $serendipity['authorid']) {
        return;
    }

    $infile = $outfile = $serendipity['serendipityPath'] . $serendipity['uploadPath'] . $file['path'] . $file['name'] . '.' . $file['extension'];

    if ($serendipity['magick'] !== true) {
        serendipity_resize_image_gd($infile, $outfile, $width, $height);
    } else {
        $cmd = escapeshellcmd($serendipity['convert']) . ' -scale ' .  serendipity_escapeshellarg($width . 'x' . $height) . ' ' . serendipity_escapeshellarg($infile) . ' ' . serendipity_escapeshellarg($outfile);
        exec($cmd, $output, $result);
        if ( $result != 0 ) {
            echo '<div class="serendipityAdminMsgError">'. sprintf(IMAGICK_EXEC_ERROR, $cmd, $output[0], $result) .'</div>';
        }
        unset($output, $result);
    }

    serendipity_updateImageInDatabase(array('dimensions_width' => $width, 'dimensions_height' => $height), $id);
    return true;
}

/**
 * Rotate an Image
 **/
function serendipity_rotateImg($id, $degrees) {
    global $serendipity;

    $file = serendipity_fetchImageFromDatabase($id);

    $admin = '';
    if (!serendipity_checkPermission('adminImagesMaintainOthers') && $file['authorid'] != '0' && $file['authorid'] != $serendipity['authorid']) {
        // A non-admin user may not delete private files from other users.
        return false;
    }

    $infile = $outfile = $serendipity['serendipityPath'] . $serendipity['uploadPath'] . $file['path'] . $file['name'] . '.' . $file['extension'];
    $infileThumb = $outfileThumb = $serendipity['serendipityPath'] . $serendipity['uploadPath'] . $file['path'] . $file['name'] . (!empty($file['thumbnail_name']) ? '.' . $file['thumbnail_name'] : '') . '.' . $file['extension'];

    if ($serendipity['magick'] !== true) {
        serendipity_rotate_image_gd($infile, $outfile, $degrees);
        serendipity_rotate_image_gd($infileThumb, $outfileThumb, $degrees);
    } else {
        /* Why can't we just all agree on the rotation direction? */
        $degrees = (360 - $degrees);

        /* Resize main image */
        $cmd = escapeshellcmd($serendipity['convert']) . ' -rotate ' . serendipity_escapeshellarg($degrees) . ' ' . serendipity_escapeshellarg($infile) . ' ' . serendipity_escapeshellarg($outfile);
        exec($cmd, $output, $result);
        if ( $result != 0 ) {
            echo '<div class="serendipityAdminMsgError">'. sprintf(IMAGICK_EXEC_ERROR, $cmd, $output[0], $result) .'</div>';
        }
        unset($output, $result);

        /* Resize thumbnail */
        $cmd = escapeshellcmd($serendipity['convert']) . ' -rotate ' . serendipity_escapeshellarg($degrees) . ' ' . serendipity_escapeshellarg($infileThumb) . ' ' . serendipity_escapeshellarg($outfileThumb);
        exec($cmd, $output, $result);
        if ( $result != 0 ) {
            echo '<div class="serendipityAdminMsgError">'. sprintf(IMAGICK_EXEC_ERROR, $cmd, $output[0], $result) .'</div>';
        }
        unset($output, $result);

    }

    $fdim = @getimagesize($outfile);

    serendipity_updateImageInDatabase(array('dimensions_width' => $fdim[0], 'dimensions_height' => $fdim[1]), $id);

    return true;
}


/**
* Creates thumbnails for all images in the upload dir
**/
function serendipity_generateThumbs() {
    global $serendipity;

    $i=0;
    $serendipity['imageList'] = serendipity_fetchImagesFromDatabase(0, 0, $total);

    foreach ($serendipity['imageList'] as $k => $file) {
        $is_image = serendipity_isImage($file);

        if ($is_image && !$file['hotlink']) {
            $update   = false;
            $filename = $file['path'] . $file['name'] .'.'. $file['extension'];
            $ffull    = $serendipity['serendipityPath'] . $serendipity['uploadPath'] . $filename;

            if (!file_exists($ffull)) {
                serendipity_deleteImage($file['id']);
                continue;
            }

            if (empty($file['thumbnail_name'])) {
                $file['thumbnail_name'] = $serendipity['thumbSuffix'];
            }

            $oldThumb = $serendipity['serendipityPath'] . $serendipity['uploadPath'] . $file['path'] . $file['name'] . '.' . $file['thumbnail_name'] . '.' . $file['extension'];
            $newThumb = $serendipity['serendipityPath'] . $serendipity['uploadPath'] . $file['path'] . $file['name'] . '.' . $serendipity['thumbSuffix'] . '.' . $file['extension'];
            $fdim = @getimagesize($ffull);

            if (!file_exists($oldThumb) && !file_exists($newThumb) && ($fdim[0] > $serendipity['thumbSize'] || $fdim[1] > $serendipity['thumbSize'])) {
                $returnsize = serendipity_makeThumbnail($file['name'] . '.' . $file['extension'], $file['path']);
                if ($returnsize !== false ) {
                    printf(RESIZE_BLAHBLAH, $filename . ': ' . $returnsize[0] . 'x' . $returnsize[1]);
                    if (!file_exists($newThumb)) {
                        printf('<span class="serendipityAdminMsgError">' . THUMBNAIL_FAILED_COPY . '</span><br />', $filename);
                    } else {
                        $update = true;
                    }
                }
            } elseif (!file_exists($oldThumb) && !file_exists($newThumb) && $fdim[0] <= $serendipity['thumbSize'] && $fdim[1] <= $serendipity['thumbSize']) {
                $res = @copy($ffull, $newThumb);
                if (@$res === true) {
                    printf(THUMBNAIL_USING_OWN . '<br />', $filename);
                    $update = true;
                } else {
                    printf('<span class="serendipityAdminMsgError">' . THUMBNAIL_FAILED_COPY . '</span><br />', $filename);
                }
            }

            if ($update) {
                $i++;
                $updates = array('thumbnail_name' => $serendipity['thumbSuffix']);
                serendipity_updateImageInDatabase($updates, $file['id']);
            }
        } else {
            // Currently, non-image files have no thumbnail.
        }
    }

    return $i;
}

function serendipity_guessMime($extension) {
    $mime = '';
    switch (strtolower($extension)) {
        case 'jpg':
        case 'jpeg':
            $mime = 'image/jpeg';
        break;

        case 'aiff':
        case 'aif':
            $mime = 'audio/x-aiff';
            break;

        case 'gif':
            $mime = 'image/gif';
        break;

        case 'png':
            $mime = 'image/png';
        break;

        case 'pdf':
            $mime = 'application/pdf';
            break;

        case 'doc':
            $mime = 'application/msword';
            break;

        case 'rtf':
            $mime = 'application/rtf';
            break;

        case 'wav':
        case 'wave':
            $mime = 'audio/x-wav';
            break;

        case 'mp2':
        case 'mpg':
        case 'mpeg':
            $mime = 'video/x-mpeg';
            break;

        case 'avi':
            $mime = 'video/x-msvideo';
            break;

        case 'mp3':
            $mime = 'audio/x-mpeg3';
            break;

        case 'xlm':
        case 'xlb':
        case 'xll':
        case 'xla':
        case 'xlw':
        case 'xlc':
        case 'xls':
        case 'xlt':
            $mime = 'application/vnd.ms-excel';
            break;

        case 'ppt':
        case 'pps':
            $mime = 'application/vnd.ms-powerpoint';
            break;

        case 'html':
        case 'htm':
            $mime = 'text/html';
            break;

        case 'xsl':
        case 'xslt':
        case 'xml':
        case 'wsdl':
        case 'xsd':
            $mime = 'text/xml';
            break;

        case 'zip':
            $mime = 'application/zip';
            break;

        case 'tar':
            $mime = 'application/x-tar';
            break;

        case 'tgz':
        case 'gz':
            $mime = 'application/x-gzip';
            break;

        case 'swf':
            $mime = 'application/x-shockwave-flash';
            break;

        case 'rm':
        case 'ra':
        case 'ram':
            $mime = 'application/vnd.rn-realaudio';
            break;

        case 'exe':
            $mime = 'application/octet-stream';
            break;

        case 'mov':
        case 'qt':
            $mime = 'video/x-quicktime';
            break;

        case 'midi':
        case 'mid':
            $mime = 'audio/x-midi';
            break;

        case 'txt':
            $mime = 'text/plain';
            break;

        case 'qcp':
            $mime = 'audio/vnd.qcelp';
            break;

        case 'emf':
            $mime = 'image/x-emf';
            break;

        case 'wmf':
            $mime = 'image/x-wmf';
            break;

        case 'snd':
            $mime = 'audio/basic';
            break;

        case 'pmd':
            $mime = 'application/x-pmd';
            break;

        case 'wbmp':
            $mime = 'image/vnd.wap.wbmp';
            break;

        case 'gcd':
            $mime = 'text/x-pcs-gcd';
            break;

        case 'mms':
            $mime = 'application/vnd.wap.mms-message';
            break;

        case 'ogg':
        case 'ogm':
            $mime = 'application/ogg';
            break;

        case 'rv':
            $mime = 'video/vnd.rn-realvideo';
            break;

        case 'wmv':
            $mime = 'video/x-ms-wmv';
            break;

        case 'wma':
            $mime = 'audio/x-ms-wma';
            break;

        case 'qcp':
            $mime = 'audio/vnd.qcelp';
            break;

        case 'jad':
            $mime = 'text/vnd.sun.j2me.app-descriptor';
            break;

        case '3g2':
        case '3gp':
            $mime = 'video/3gpp';
            break;

        case 'jar':
            $mime = 'application/java-archive';

        default:
            $mime = 'application/octet-stream';
            break;
    }

    return $mime;
}

/**
* Creates thumbnails for all images in the upload dir
**/
function serendipity_syncThumbs() {
    global $serendipity;

    $i=0;
    $files = serendipity_fetchImages();

    $fcount = count($files);
    for ($x = 0; $x < $fcount; $x++) {
        $update = $q = array();
        $f      = serendipity_parseFileName($files[$x]);
        if (empty($f[1]) || $f[1] == $files[$x]) {
            // No extension means bad file most probably. Skip it.
            printf(SKIPPING_FILE_EXTENSION . '<br />', $files[$x]);
            continue;
        }

        $ffull   = $serendipity['serendipityPath'] . $serendipity['uploadPath'] . $files[$x];
        $fthumb  = $serendipity['serendipityPath'] . $serendipity['uploadPath'] . $f[0] . '.' . $serendipity['thumbSuffix'] . '.' . $f[1];
        $fbase   = basename($f[0]);
        $fdir    = dirname($f[0]) . '/';
        if ($fdir == './') {
            $fdir = '';
        }

        if (!is_readable($ffull) || filesize($ffull) == 0) {
            printf(SKIPPING_FILE_UNREADABLE . '<br />', $files[$x]);
            continue;
        }

        $ft_mime = serendipity_guessMime($f[1]);
        $fdim    = serendipity_getimagesize($ffull, $ft_mime);

        $rs = serendipity_db_query("SELECT * FROM {$serendipity['dbPrefix']}images
                                            WHERE name = '" . serendipity_db_escape_string($fbase) . "'
                                              " . ($fdir != '' ? "AND path = '" . serendipity_db_escape_string($fdir) . "'" : '') . "
                                              AND mime = '" . serendipity_db_escape_string($fdim['mime']) . "'", true, 'assoc');
        if (is_array($rs)) {
            $update    = array();
            $checkfile = $serendipity['serendipityPath'] . $serendipity['uploadPath'] . $rs['path'] . $rs['name'] . '.' . $rs['thumbnail_name'] . '.' . $rs['extension'];
            if (isset($fdim[0]) && $rs['dimensions_width'] != $fdim[0]) {
                $update['dimensions_width'] = $fdim[0];
            }

            if (isset($fdim[1]) && $rs['dimensions_height'] != $fdim[1]) {
                $update['dimensions_height'] = $fdim[1];
            }

            if ($rs['size'] != filesize($ffull)) {
                $update['size'] = filesize($ffull);
            }

            if (!file_exists($checkfile) && file_exists($fthumb)) {
                $update['thumbnail_name'] = $serendipity['thumbSuffix'];
            }

            /* Do the database update, if needed */
            if (sizeof($update) != 0) {
                printf(FOUND_FILE . '<br />', $files[$x]);
                serendipity_updateImageInDatabase($update, $rs['id']);
                $i++;
            }
        } else {
            printf(FOUND_FILE . '<br />', $files[$x]);
            serendipity_insertImageInDatabase($fbase . '.' . $f[1], $fdir, 0, filemtime($ffull));
            $i++;
        }
    }
    return $i;
}

function serendipity_functions_gd($infilename) {
    if (!function_exists('imagecopyresampled')) {
        return false;
    }

    $func = array();
    $inf  = pathinfo(strtolower($infilename));
    switch ($inf['extension']) {
    case 'gif':
        $func['load'] = 'imagecreatefromgif';
        $func['save'] = 'imagegif';
        break;

    case 'jpeg':
    case 'jpg':
    case 'jfif':
        $func['load'] = 'imagecreatefromjpeg';
        $func['save'] = 'imagejpeg';
        break;

    case 'png':
        $func['load'] = 'imagecreatefrompng';
        $func['save'] = 'imagepng';
        break;

    default:
        return false;
    }

    /* If our loader does not exist, we are doomed */
    if (!function_exists($func['load'])) {
        return false;
    }

    /* If the save function does not exist (i.e. read-only GIF), we want to output it as PNG */
    if (!function_exists($func['save'])) {
        if (function_exists('imagepng')) {
            $func['save'] = 'imagepng';
        } else {
            return false;
        }
    }

    return $func;
}

function serendipity_rotate_image_gd($infilename, $outfilename, $degrees)
{
    $func = serendipity_functions_gd($infilename);
    if (!is_array($func)) {
        return false;
    }

    $in        = $func['load']($infilename);

    $out       = imagerotate($in, $degrees, 0);
    $func['save']($out, $outfilename);

    $newwidth  = imagesx($out);
    $newheight = imagesy($out);

    $out       = null;
    $in        = null;

    return array($newwidth, $newheight);
}


function serendipity_resize_image_gd($infilename, $outfilename, $newwidth, $newheight=null)
{
    $func = serendipity_functions_gd($infilename);
    if (!is_array($func)) {
        return false;
    }

    $in = $func['load']($infilename);
    $width = imagesx($in);
    $height = imagesy($in);

    if (is_null($newheight)) {
        $newsizes = serendipity_calculate_aspect_size($width, $height, $newwidth);
        $newwidth = $newsizes[0];
        $newheight = $newsizes[1];
    }

    $out = imagecreatetruecolor($newwidth, $newheight);

    /* Attempt to copy transparency information, this really only works for PNG */
    if (function_exists('imagesavealpha')) {
        imagealphablending($out, false);
        imagesavealpha($out, true);
    }

    imagecopyresampled($out, $in, 0, 0, 0, 0, $newwidth, $newheight, $width, $height);
    $func['save']($out, $outfilename, 100);
    $out = null;
    $in  = null;

    return array($newwidth, $newheight);
}

function serendipity_calculate_aspect_size($width, $height, $newwidth) {

    // calculate aspect ratio
    $div_width  = $width  / $newwidth;
    $div_height = $height / $newwidth;

    if ($div_width <= 1 && $div_height <= 1) {
        // do not scale small images where both sides are smaller than the thumbnail dimensions
        $newheight = $height;
        $newwidth  = $width;
    } elseif ($div_width >= $div_height) {
        // max width - calculate height, keep width as scaling base
        $newheight = round($height / $div_width);
        // make sure the height is at least 1 pixel for extreme images
        $newheight = ($newheight >= 1 ? $newheight : 1);
    } else {
        // max height - calculate width, keep height as scaling base
        $newheight = $newwidth;
        $newwidth  = round($width / $div_height);
        // make sure the width is at least 1 pixel for extreme images
        $newwidth  = ($newwidth >= 1 ? $newwidth : 1);
    }

    return array($newwidth, $newheight);
}

function serendipity_displayImageList($page = 0, $lineBreak = NULL, $manage = false, $url = NULL, $show_upload = false, $limit_path = NULL) {
    global $serendipity;
    $sort_row_interval = array(8, 16, 50, 100);
    $sortParams        = array('perpage', 'order', 'ordermode');
    $importParams      = array('adminModule', 'htmltarget', 'filename_only', 'textarea', 'subpage');
    $extraParems       = '';
    $filterParams      = array('only_path', 'only_filename');

    foreach($importParams AS $importParam) {
        if (isset($serendipity['GET'][$importParam])) {
            $extraParems .= 'serendipity[' . $importParam . ']='. $serendipity['GET'][$importParam] .'&amp;';
        }
    }

    foreach($sortParams AS $sortParam) {
        serendipity_restoreVar($serendipity['COOKIE']['sortorder_' . $sortParam], $serendipity['GET']['sortorder'][$sortParam]);
        serendipity_JSsetCookie('sortorder_' . $sortParam, $serendipity['GET']['sortorder'][$sortParam]);
        $extraParems .= 'serendipity[sortorder]['. $sortParam .']='. $serendipity['GET']['sortorder'][$sortParam] .'&amp;';
    }

    foreach($filterParams AS $filterParam) {
        serendipity_restoreVar($serendipity['COOKIE'][$filterParam], $serendipity['GET'][$filterParam]);
        serendipity_JSsetCookie($filterParam, $serendipity['GET'][$filterParam]);
        if (!empty($serendipity['GET'][$filterParam])) {
            $extraParems .= 'serendipity[' . $filterParam . ']='. $serendipity['GET'][$filterParam] .'&amp;';
        }
    }

    $serendipity['GET']['only_path']     = serendipity_uploadSecure($limit_path . $serendipity['GET']['only_path'], true);
    $serendipity['GET']['only_filename'] = str_replace(array('*', '?'), array('%', '_'), $serendipity['GET']['only_filename']);

    $perPage = (!empty($serendipity['GET']['sortorder']['perpage']) ? $serendipity['GET']['sortorder']['perpage'] : $sort_row_interval[0]);
    $start   = ($page-1) * $perPage;

    $serendipity['imageList'] = serendipity_fetchImagesFromDatabase(
                                  $start,
                                  $perPage,
                                  $totalImages, // Passed by ref
                                  (isset($serendipity['GET']['sortorder']['order']) ? $serendipity['GET']['sortorder']['order'] : false),
                                  (isset($serendipity['GET']['sortorder']['ordermode']) ? $serendipity['GET']['sortorder']['ordermode'] : false),
                                  (isset($serendipity['GET']['only_path']) ? $serendipity['GET']['only_path'] : ''),
                                  (isset($serendipity['GET']['only_filename']) ? $serendipity['GET']['only_filename'] : '')
    );

    $pages         = ceil($totalImages / $perPage);
    $linkPrevious  = '?' . $extraParems . 'serendipity[page]=' . ($page-1);
    $linkNext      = '?' . $extraParems . 'serendipity[page]=' . ($page+1);
    $sort_order    = serendipity_getImageFields();
    $paths         = serendipity_traversePath($serendipity['serendipityPath'] . $serendipity['uploadPath']. $limit_path);

    if (is_null($lineBreak)) {
        $lineBreak = floor(750 / ($serendipity['thumbSize'] + 20));
    }
?>
<form style="display: inline; margin: 0px; padding: 0px;" method="get" action="?">
<?php
    echo serendipity_setFormToken();
    foreach($serendipity['GET'] AS $g_key => $g_val) {
        if ( !is_array($g_val) && $g_key != 'page' ) {
            echo '<input type="hidden" name="serendipity[' . $g_key . ']" value="' . htmlspecialchars($g_val) . '" />';
        }
    }
?>
    <table class="serendipity_admin_filters" width="100%">
        <tr>
            <td class="serendipity_admin_filters_headline" colspan="6"><strong><?php echo FILTERS ?></strong> - <?php echo FIND_MEDIA ?></td>
        </tr>
        <tr>
            <td><?php echo FILTER_DIRECTORY ?></td>
            <td><select name="serendipity[only_path]">
                    <option value=""> <?php if (!$limit_path) { echo ALL_DIRECTORIES; } else { echo basename($limit_path);}?></option>
                    <?php foreach ( $paths as $folder ) { ?>
                    <option <?php echo ($serendipity['GET']['only_path'] == $limit_path.$folder['relpath']) ? 'selected="selected"' : '' ?> value="<?php echo $folder['relpath'] ?>"><?php echo str_repeat('&nbsp;', $folder['depth']*2) . ' '. $folder['name'] ?></option>
                    <?php } ?>
                </select>
            </td>
            <td><?php echo SORT_ORDER_NAME ?></td>
            <td colspan="3"><input type="text" name="serendipity[only_filename]" value="<?php echo htmlspecialchars($serendipity['GET']['only_filename']); ?>" /></td>
        </tr>
        <tr>
            <td class="serendipity_admin_filters_headline" colspan="6"><strong><?php echo SORT_ORDER ?></strong></td>
        </tr>
        <tr>
            <td><?php echo SORT_BY ?></td>
            <td><select name="serendipity[sortorder][order]">
<?php
        foreach($sort_order AS $so_key => $so_val) {
            echo '<option value="' . $so_key . '" ' . (isset($serendipity['GET']['sortorder']['order']) && $serendipity['GET']['sortorder']['order'] == $so_key ? 'selected="selected"' : '') . '>' . $so_val . '</option>';
        }
?>              </select>
</td>
            <td><?php echo SORT_ORDER ?></td>
            <td><select name="serendipity[sortorder][ordermode]">
                    <option value="DESC" <?php echo (isset($serendipity['GET']['sortorder']['ordermode']) && $serendipity['GET']['sortorder']['ordermode'] == 'DESC' ? 'selected="selected"' : '') ?>><?php echo SORT_ORDER_DESC ?></option>
                    <option value="ASC"  <?php echo (isset($serendipity['GET']['sortorder']['ordermode']) && $serendipity['GET']['sortorder']['ordermode'] == 'ASC'  ? 'selected="selected"' : '') ?>><?php echo SORT_ORDER_ASC  ?></option>
                </select>
            </td>
            <td><?php echo FILES_PER_PAGE ?></td>
            <td><select name="serendipity[sortorder][perpage]">
<?php
        foreach($sort_row_interval AS $so_val) {
            echo '<option value="' . $so_val . '" ' . ($perPage == $so_val ? 'selected="selected"' : '') . '>' . $so_val . '</option>';
        }
?>              </select>
            </td>
        </tr>
        <tr>
            <td align="right" colspan="6">
<?php
        if ($show_upload) {
?>
                <input type="button" value="<?php echo htmlspecialchars(ADD_MEDIA); ?>" onclick="location.href='<?php echo $url; ?>&amp;serendipity[adminAction]=addSelect'; return false" class="serendipityPrettyButton" />
<?php
        }
?>
                <input type="submit" name="go" value=" - <?php echo GO ?> - " class="serendipityPrettyButton" />
            </td>
        </tr>
</table>
</form>
<?php if ( sizeof($serendipity['imageList']) == 0 ) { ?>
    <div align="center">- <?php echo NO_IMAGES_FOUND ?> -</div>
<?php } else { ?>
<table border="0" width="100%">
    <tr>
        <td colspan="<?php echo floor($lineBreak); ?>">
            <table width="100%">
                <tr>
                    <td>
                    <?php if ( $page != 1 && $page <= $pages ) { ?>
                        <a href="<?php echo $linkPrevious ?>" class="serendipityIconLink"><img src="<?php echo serendipity_getTemplateFile('admin/img/previous.png') ?>" /><?php echo PREVIOUS ?></a>
                    <?php } ?></td>
                    <td align="right">
                    <?php if ($page != $pages ) { ?>
                        <a href="<?php echo $linkNext ?>" class="serendipityIconLinkRight"><?php echo NEXT ?><img src="<?php echo serendipity_getTemplateFile('admin/img/next.png') ?>" /></a>
                    <?php } ?></td>
                </tr>
            </table>
        </td>
    </tr>
    <tr>
<?php
        $x = 0;
        foreach ($serendipity['imageList'] as $k => $file) {
            ++$x; $preview = '';
            $img = $serendipity['serendipityPath'] . $serendipity['uploadPath'] . $file['path'] . $file['name'] . (!empty($file['thumbnail_name']) ? '.' . $file['thumbnail_name'] : '') . '.' . $file['extension'];
            $i = @getimagesize($img);
            $file['imgsrc'] = $serendipity['uploadHTTPPath'] . $file['path'] . $file['name'] . (!empty($file['thumbnail_name']) ? '.' . $file['thumbnail_name'] : '') . '.' . $file['extension'];
            $is_image = serendipity_isImage($file);

            if (!($serendipity['authorid'] == $file['authorid'] || $file['authorid'] == '0' || serendipity_checkPermission('adminImagesViewOthers'))) {
                // This is a fail-safe continue. Basically a non-matching file should already be filtered in SQL.
                continue;
            }

            /* If it is an image, and the thumbnail exists */
            if ($is_image && file_exists($img)) {
                $preview .= '<img src="' . $serendipity['serendipityHTTPPath'] . $file['imgsrc'] . '" border="0" title="' . $file['path'] . $file['name'] . '" alt="'. $file['name'] . '" />';
                if ($url) {
                    $preview = '<a href="'. $url .'&amp;serendipity[image]='. $file['id'] .'">'. $preview .'</a>';
                }
            } elseif ($is_image && $file['hotlink']) {
                $sizes = serendipity_calculate_aspect_size($file['dimensions_width'], $file['dimensions_height'], $serendipity['thumbSize']);
                $preview .= '<img src="' . $file['path'] . '" width="' . $sizes[0] . '" height="' . $sizes[1] . '" border="0" title="' . $file['path'] . '" alt="'. $file['name'] . '" />';
                if ($url) {
                    $preview = '<a href="'. $url .'&amp;serendipity[image]='. $file['id'] .'">'. $preview .'</a>';
                }
            /* If it's not an image, or the thumbnail does not exist */
            } else {
                $preview .= '<img src="'. serendipity_getTemplateFile('admin/img/mime_unknown.png') .'" title="' . $file['path'] . $file['name'] . ' (' . $file['mime'] . ')" alt="'. $file['mime'] .'" /><br /><span style="font-weight: bold; font-size: 8pt">- ' . (($file['hotlink']) ? MEDIA_HOTLINKED : $file['mime']) .' -</span>';
                if ($url) {
                    $preview .= '<br /><a href="' . $url . '&amp;serendipity[image]=' . $file['id'] . '">' . $file['name'] . '.' . $file['extension'] . '</a>';
                }
                $preview .= '</div>';
            }

?>
                <td nowrap="nowrap" align="center" valign="<?php echo $manage ? 'top' : 'middle' ?>" width="<?php echo round(1/$lineBreak*100) ?>%" class="serendipity_admin_list_item serendipity_admin_list_item_<?php echo (($i % 2) ? 'even' : 'uneven') ?>">
<?php
if ( !$manage ) {
    echo $preview;
} else { ?>
                    <table width="100%" border="0" cellspacing="0" cellpadding="3">
                        <tr>
                            <td valign="top" width="16" rowspan="3">
<?php
                if ($serendipity['authorid'] == $file['authorid'] || $file['authorid'] == '0' || serendipity_checkPermission('adminImagesMaintainOthers')) {
                    $popupWidth = ($is_image ? ($file['dimensions_width'] + 20) : 600);
                    $popupHeight = ($is_image ? ($file['dimensions_height'] + 20) : 500);
?>
                            <img class="serendipityImageButton" title="<?php echo MEDIA_FULLSIZE; ?>" alt="<?php echo MEDIA_FULLSIZE; ?>" src="<?php echo serendipity_getTemplateFile('admin/img/big_zoom.png') ?>"   border="0" onclick="F1 = window.open('<?php echo ($file['hotlink'] ? $file['path'] : $serendipity['serendipityHTTPPath'] . $serendipity['uploadHTTPPath'] . $file['path'] . $file['name'] . '.'. $file['extension']); ?>','Zoom','height=<?php echo $popupHeight; ?>,width=<?php echo $popupWidth; ?>,top='+ (screen.height-<?php echo $popupHeight ?>)/2 +',left='+ (screen.width-<?php echo $popupWidth ?>)/2 +',toolbar=no,menubar=no,location=no,resize=1,resizable=1<?php echo ($is_image ? '' : ',scrollbars=yes'); ?>');" /><br />
                            <img class="serendipityImageButton" title="<?php echo MEDIA_RENAME; ?>" alt="<?php echo MEDIA_RENAME; ?>" src="<?php echo serendipity_getTemplateFile('admin/img/big_rename.png') ?>" border="0" onclick="rename('<?php echo $file['id']; ?>', '<?php echo addslashes($file['name']); ?>')" /><br />
                            <?php if ($is_image && !$file['hotlink']) { ?><img class="serendipityImageButton" title="<?php echo IMAGE_RESIZE; ?>"   alt="<?php echo IMAGE_RESIZE; ?>"   src="<?php echo serendipity_getTemplateFile('admin/img/big_resize.png') ?>" border="0" onclick="location.href='?serendipity[adminModule]=images&amp;serendipity[adminAction]=scaleSelect&amp;serendipity[fid]=<?php echo $file['id']; ?>';" /><br /><?php } ?>
                            <?php if ($is_image && !$file['hotlink']) { ?><a href="?serendipity[adminModule]=images&amp;serendipity[adminAction]=rotateCCW&amp;serendipity[fid]=<?php echo $file['id']; ?>"><img class="serendipityImageButton" title="<?php echo IMAGE_ROTATE_LEFT; ?>" alt="<?php echo IMAGE_ROTATE_LEFT; ?>" src="<?php echo serendipity_getTemplateFile('admin/img/big_rotate_ccw.png') ?>" border="0" /><br /><?php } ?>
                            <?php if ($is_image && !$file['hotlink']) { ?><a href="?serendipity[adminModule]=images&amp;serendipity[adminAction]=rotateCW&amp;serendipity[fid]=<?php echo $file['id']; ?>"><img class="serendipityImageButton" title="<?php echo IMAGE_ROTATE_RIGHT; ?>" alt="<?php echo IMAGE_ROTATE_RIGHT; ?>" src="<?php echo serendipity_getTemplateFile('admin/img/big_rotate_cw.png') ?>" border="0" /><br /><?php } ?>
                            <a href="?serendipity[adminModule]=images&amp;serendipity[adminAction]=delete&amp;serendipity[fid]=<?php echo $file['id']; ?>"><img class="serendipityImageButton" title="<?php echo MEDIA_DELETE; ?>"   alt="<?php echo MEDIA_DELETE; ?>"   src="<?php echo serendipity_getTemplateFile('admin/img/big_delete.png') ?>" border="0" /><br />
<?php
                }
?>
                            </td>
                            <td height="10" align="left" style="font-weight: bold; font-size: 8pt"><?php echo $file['name'] . '.' . $file['extension']; ?></td>
                            <td height="10" align="right" style="font-size: 8pt"><?php echo ($file['authorid'] == '0' ? ALL_AUTHORS : $file['authorname']); ?></td>
                        </tr>
                        <tr>
                            <td align="center" colspan="2"><?php echo $preview ?></td>
                        </tr>
                        <tr>
                            <td colspan="2" height="10" align="center" style="font-size: 8pt">
<?php
                if ($is_image && !$file['hotlink']) {
                    echo ORIGINAL_SHORT . ': ' . $file['dimensions_width'] . 'x' . $file['dimensions_height'] .', ';
                    echo THUMBNAIL_SHORT . ': ' . $i[0] . 'x' . $i[1];
                } elseif ($file['hotlink']) {
                    echo wordwrap($file['path'], 45, '<br />', 1);
                } else {
                    echo SORT_ORDER_SIZE . ': ' . number_format(round($file['size']/1024, 2), NUMBER_FORMAT_DECIMALS, NUMBER_FORMAT_DECPOINT, NUMBER_FORMAT_THOUSANDS) . 'kb';
                }
?>
                            </td>
                        </tr>
                    </table>
<?php } ?>
        </td>
<?php
        // Newline?
        if ($x % $lineBreak == 0) {
?>
    </tr>
    <tr>
<?php
        }
    }
?>
    </tr>
</table>
<?php
}
} // End serendipity_displayImageList()

function serendipity_isImage(&$file, $strict = false) {
    global $serendipity;

    $file['displaymime'] = $file['mime'];

    // Strip HTTP path out of imgsrc
    $file['location'] = $serendipity['serendipityPath'] . preg_replace('@^(' . preg_quote($serendipity['serendipityHTTPPath']) . ')@i', '', $file['imgsrc']);

    // File is PDF -> Thumb is PNG
    if ($file['mime'] == 'application/pdf' && file_exists($file['location'] . '.png') && $strict == false) {
        $file['imgsrc']     .= '.png';
        $file['displaymime'] = 'image/png';
    }

    return (0 === strpos(strtolower($file['displaymime']), 'image/'));
}

function serendipity_killPath($basedir, $directory = '', $forceDelete = false) {
    static $n = "<br />\n";
    static $serious = true;

    if ($handle = @opendir($basedir . $directory)) {
        while (false !== ($file = @readdir($handle))) {
            if ($file != '.' && $file != '..') {
                if (is_dir($basedir . $directory . $file)) {
                    serendipity_killPath($basedir, $directory . $file . '/', $forceDelete);
                } else {
                    $filestack[$file] = $directory . $file;
                }
            }
        }
        @closedir($handle);

        printf(CHECKING_DIRECTORY . "<br />\n", $directory);

        // No, we just don't kill files the easy way. We sort them out properly from the database
        // and preserve files not entered therein.
        $files = serendipity_fetchImagesFromDatabase(0, 0, $total, false, false, $directory);
        if (is_array($files)) {
            echo "<ul>\n";
            foreach($files AS $f => $file) {
                echo "<li>\n";
                if ($serious) {
                    serendipity_deleteImage($file['id']);
                } else {
                    echo $file['name'] . '.' . $file['extension'];
                }
                echo "</li>\n";

                unset($filestack[$file['name'] . '.' . $file['extension']]);
                unset($filestack[$file['name'] . (!empty($file['thumbnail_name']) ? '.' . $file['thumbnail_name'] : '') . '.' . $file['extension']]);
            }
            echo "</ul>\n";
        }

        if (count($filestack) > 0) {
            if ($forceDelete) {
                echo "<ul>\n";
                foreach($filestack AS $f => $file) {
                    if ($serious && @unlink($basedir . $file)) {
                        printf('<li>' . DELETING_FILE . $n . DONE . "</li>\n", $file);
                    } else {
                        printf('<li>' . DELETING_FILE . $n . ERROR . "</li>\n", $file);
                    }
                }
                echo "</ul>\n";
            } else {
                echo ERROR_DIRECTORY_NOT_EMPTY . $n;
                echo "<ul>\n";
                foreach($filestack AS $f => $file) {
                    echo '<li>' . $file . "</li>\n";
                }
                echo "</ul>\n";
            }
        }

        echo '<strong>';
        if ($serious && !empty($directory) && !preg_match('@^.?/?$@', $directory) && @rmdir($basedir . $directory)) {
            printf(DIRECTORY_DELETE_SUCCESS . $n, $directory);
        } else {
            printf(DIRECTORY_DELETE_FAILED . $n, $directory);
        }
        echo '</strong>';
    }

    return true;
}


function serendipity_traversePath($basedir, $dir='', $onlyDirs=true, $pattern = NULL, $depth = 1, $max_depth = null) {


    $dh = @opendir($basedir . '/' . $dir);
    if ( !$dh ) {
        return array();
    }

    $files = array();
    while (($file = @readdir($dh)) !== false) {
        if ( $file != '.' && $file != '..' ) {
            if ( $onlyDirs === false || ($onlyDirs === true && is_dir($basedir . '/' . $dir . '/' . $file)) ) {
                if ( is_null($pattern) || preg_match($pattern, $file) ) {
                    $files[] = array(
                        'name'    => $file,
                        'depth'   => $depth,
                        'relpath' => ltrim(str_replace('\\', '/', $dir) . basename($file) . '/', '/')
                    );
                }
            }
            if ( is_dir($basedir . '/' . $dir . '/' . $file) && ($max_depth === null || $depth < $max_depth)) {
                $files = array_merge($files, serendipity_traversePath($basedir, $dir . '/' . basename($file) . '/', $onlyDirs, $pattern, ($depth+1), $max_depth));
            }
        }
    }

    @closedir($dh);
    return $files;
}


function serendipity_deletePath($dir) {
    $d = dir($dir);
    if ($d) {
        while ($f = $d->read() ){
            if ($f != '.' && $f != '..') {
                if (is_dir($dir . $f)){
                    serendipity_deletePath($dir . $f . '/');
                    rmdir($dir . $f);
                }

                if (is_file($dir . $f)) {
                    unlink($dir . $f);
                }
            }
        }

        $d->close();
    }
}

function serendipity_uploadSecure($var, $strip_paths = true, $append_slash = false) {
    $var = preg_replace('@[^0-9a-z\._/-]@i', '', $var);
    if ($strip_paths) {
        $var = preg_replace('@(\.+[/\\\\]+)@', '/', $var);
    }

    $var = preg_replace('@^(/+)@', '', $var);
    
    if ($append_slash) {
        if (!empty($var) && substr($var, -1, 1) != '/') {
            $var .= '/';
        }
    }

    return $var;
}

function serendipity_getimagesize($file, $ft_mime = '', $suf = '') {
    if (empty($ft_mime) && !empty($suf)) {
        $ft_mime = serendipity_guessMime($suf);
    }

    if ($ft_mime == 'application/pdf') {
        $fdim = array(1000,1000,24, '', 'bits'=> 24, 'channels' => '3', 'mime' => 'application/pdf');
    } else {
        $fdim = @getimagesize($file);
    }

    if (is_array($fdim)) {
        if (empty($fdim['mime'])) {
            $fdim['mime'] = $ft_mime;
        }

        if ($fdim['mime'] == 'image/vnd.wap.wbmp' && $ft_mime == 'video/x-quicktime') {
            // PHP Versions prior to 4.3.9 reported .mov files wrongly as WAP. Fix this and mark the file as 'non-image' with 0x0 dimensions
            $fdim['mime'] = $ft_mime;
        }
    } else {
        // The file is no image. Return a fake array so that files are inserted (but without a thumb)
        $fdim = array(
            0         => 0,
            1         => 0,
            'mime'    => $ft_mime,
            'noimage' => true
        );
    }

    return $fdim;
}

function serendipity_getImageFields() {
    return array(
        'date'              => SORT_ORDER_DATE,
        'name'              => SORT_ORDER_NAME,
        'authorid'          => AUTHOR,
        'extension'         => SORT_ORDER_EXTENSION,
        'size'              => SORT_ORDER_SIZE,
        'dimensions_width'  => SORT_ORDER_WIDTH,
        'dimensions_height' => SORT_ORDER_HEIGHT
    );
}

function serendipity_escapeshellarg($string) {
    return escapeshellarg(str_replace('%', '', $string));
}
?>
