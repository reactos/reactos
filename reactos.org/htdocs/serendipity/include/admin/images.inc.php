<?php # $Id: images.inc.php 654 2005-11-06 21:25:32Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

if (IN_serendipity !== true) {
    die ("Don't hack!");
}

if (!serendipity_checkPermission('adminImages')) {
    return;
}

switch ($serendipity['GET']['adminAction']) {
    case 'sync':
        if (!serendipity_checkPermission('adminImagesSync')) {
            break;
        }
        @set_time_limit(0);
        @ignore_user_abort();

        echo '<p><b>' . SYNCING . '</b><br /><br />';
        flush();

        $i = serendipity_syncThumbs();
        printf(SYNC_DONE, $i);

        echo '<p><b>' . RESIZING . '</b><br /><br />';
        flush();

        $i = serendipity_generateThumbs();
        printf(RESIZE_DONE, $i);

        break;

    case 'DoDelete':
        if (!serendipity_checkFormToken() || !serendipity_checkPermission('adminImagesDelete')) {
            break;
        }

        $file   = $serendipity['GET']['fname'];
        serendipity_deleteImage($serendipity['GET']['fid']);
        break;

    case 'delete':
        $file     = serendipity_fetchImageFromDatabase($serendipity['GET']['fid']);

        if (!serendipity_checkPermission('adminImagesDelete') || (!serendipity_checkPermission('adminImagesMaintainOthers') && $file['authorid'] != '0' && $file['authorid'] != $serendipity['authorid'])) {
            return;
        }

        $abortLoc = $serendipity['serendipityHTTPPath'] . 'serendipity_admin.php?serendipity[adminModule]=images';
        $newLoc   = $abortLoc . '&serendipity[adminAction]=DoDelete&serendipity[fid]=' . $serendipity['GET']['fid'] . '&' . serendipity_setFormToken('url');

        printf(ABOUT_TO_DELETE_FILE, $file['name'] .'.'. $file['extension']);
?>
    <form method="get" name="delete_image">
        <div>
              <a href="<?php echo $newLoc; ?>" class="serendipityPrettyButton"><?php echo DUMP_IT ?></a>
              &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
              <a href="<?php echo $abortLoc; ?>" class="serendipityPrettyButton"><?php echo ABORT_NOW ?></a>
        </div>
    </form>
<?php
        break;

    case 'rename':
        $file = serendipity_fetchImageFromDatabase($serendipity['GET']['fid']);
        $serendipity['GET']['newname'] = serendipity_uploadSecure($serendipity['GET']['newname'], true);

        if (!serendipity_checkFormToken() || !serendipity_checkPermission('adminImagesDelete') || (!serendipity_checkPermission('adminImagesMaintainOthers') && $file['authorid'] != '0' && $file['authorid'] != $serendipity['authorid'])) {
            return;
        }

        if (serendipity_isActiveFile(basename($serendipity['GET']['newname']))) {
            printf(ERROR_FILE_FORBIDDEN, $serendipity['GET']['newname']);
            return;
        }

        if ($file['hotlink']) {
            serendipity_updateImageInDatabase(array('name' => $serendipity['GET']['newname']), $serendipity['GET']['fid']);
        } else {
            $newfile = $serendipity['serendipityPath'] . $serendipity['uploadPath'] . $file['path'] . $serendipity['GET']['newname'] . '.' . $file['extension'];
            $oldfile = $serendipity['serendipityPath'] . $serendipity['uploadPath'] . $file['path'] . $file['name'] . '.'. $file['extension'];
            if ($serendipity['GET']['newname'] != '' && file_exists($oldfile) && !file_exists($newfile)) {
                // Rename file
                rename($oldfile, $newfile);

                // Rename thumbnail
                rename($serendipity['serendipityPath'] . $serendipity['uploadPath'] . $file['path'] . $file['name'] . (!empty($file['thumbnail_name']) ? '.' . $file['thumbnail_name'] : '') . '.' .  $file['extension'],
                       $serendipity['serendipityPath'] . $serendipity['uploadPath'] . $file['path'] . $serendipity['GET']['newname'] . '.' . $serendipity['thumbSuffix'] . '.' . $file['extension']);

                serendipity_updateImageInDatabase(array('thumbnail_name' => $serendipity['thumbSuffix'], 'name' => $serendipity['GET']['newname']), $serendipity['GET']['fid']);
                // Forward user to overview (we don't want the user's back button to rename things again)
            } else {
                if (!file_exists($oldfile)) {
                    echo ERROR_FILE_NOT_EXISTS;
                } elseif (file_exists($newfile)) {
                    echo ERROR_FILE_EXISTS;
                } else {
                    echo ERROR_SOMETHING;
                }
    ?>
        <br />
        <input type="button" onclick="history.go(-1);" value="<?php echo BACK; ?>" class="serendipityPrettyButton" />
    <?php
                break;
            }
        }

        // if we successfully rename
    ?>
        <script language="javascript" type="text/javascript">
            location.href="?serendipity[adminModule]=images";
        </script>
        <noscript>
            <a href="?serendipity[adminModule]=images"><?php echo DONE ?></a>
        </noscript>
    <?php
        break;

    case 'add':
        if (!serendipity_checkFormToken() || !serendipity_checkPermission('adminImagesAdd')) {
            return;
        }

?>
    <b><?php echo ADDING_IMAGE; ?></b>
    <br /><br />
<?php

    $authorid = (isset($serendipity['POST']['all_authors']) && $serendipity['POST']['all_authors'] == 'true') ? '0' : $serendipity['authorid'];

    // First find out whether to fetch a file or accept an upload
    if ($serendipity['POST']['imageurl'] != '' && $serendipity['POST']['imageurl'] != 'http://') {
        if (!empty($serendipity['POST']['target_filename'][2])) {
            // Faked hidden form 2 when submitting with JavaScript
            $tfile   = $serendipity['POST']['target_filename'][2];
            $tindex  = 2;
        } elseif (!empty($serendipity['POST']['target_filename'][1])) {
            // Fallback key when not using JavaScript
            $tfile   = $serendipity['POST']['target_filename'][1];
            $tindex  = 1;
        } else {
            $tfile   = $serendipity['POST']['imageurl'];
            $tindex  = 1;
        }

        $tfile = serendipity_uploadSecure(basename($tfile));

        if (serendipity_isActiveFile($tfile)) {
            printf(ERROR_FILE_FORBIDDEN, $tfile);
            break;
        }

        $serendipity['POST']['target_directory'][$tindex] = serendipity_uploadSecure($serendipity['POST']['target_directory'][$tindex], true, true);
        $target = $serendipity['serendipityPath'] . $serendipity['uploadPath'] . $serendipity['POST']['target_directory'][$tindex] . $tfile;

        if (file_exists($target)) {
            echo '(' . $target . ') ' . ERROR_FILE_EXISTS_ALREADY;
        } else {
            require_once S9Y_PEAR_PATH . 'HTTP/Request.php';
            $req = &new HTTP_Request($serendipity['POST']['imageurl']);
            // Try to get the URL

            if (PEAR::isError($req->sendRequest()) || $req->getResponseCode() != '200') {
                printf(REMOTE_FILE_NOT_FOUND, $serendipity['POST']['imageurl']);
            } else {
                // Fetch file
                $fContent = $req->getResponseBody();

                if ($serendipity['POST']['imageimporttype'] == 'hotlink') {
                    $tempfile = $serendipity['serendipityPath'] . $serendipity['uploadPath'] . '/hotlink_' . time();
                    $fp = fopen($tempfile, 'w');
                    fwrite($fp, $fContent);
                    fclose($fp);
                      
                    $image_id = @serendipity_insertHotlinkedImageInDatabase($tfile, $serendipity['POST']['imageurl'], $authorid, null, $tempfile);
                    printf(HOTLINK_DONE. '<br />', $serendipity['POST']['imageurl'], $tfile);
                    serendipity_plugin_api::hook_event('backend_image_addHotlink', $tempfile);
                } else {
                    $fp = fopen($target, 'w');
                    fwrite($fp, $fContent);
                    fclose($fp);

                    printf(FILE_FETCHED . '<br />', $serendipity['POST']['imageurl'], $tfile);

                    // Create thumbnail
                    if ( $created_thumbnail = serendipity_makeThumbnail($tfile, $serendipity['POST']['target_directory'][$tindex]) ) {
                        echo THUMB_CREATED_DONE . '<br />';
                    }
                    // Insert into database
                    $image_id = serendipity_insertImageInDatabase($tfile, $serendipity['POST']['target_directory'][$tindex], $authorid);
                    serendipity_plugin_api::hook_event('backend_image_add', $target);
                }
            }
        }
    } else {
        if (!is_array($serendipity['POST']['target_filename'])) {
            break;
        }
        
        foreach($serendipity['POST']['target_filename'] AS $idx => $target_filename) {
            $uploadfile = &$_FILES['serendipity']['name']['userfile'][$idx];
            $uploadtmp  = &$_FILES['serendipity']['tmp_name']['userfile'][$idx];
            if (!empty($target_filename)) {
                $tfile   = $target_filename;
            } elseif (!empty($uploadfile)) {
                $tfile   = $uploadfile;
            } else {
                // skip empty array
                continue;
            }
            
            $tfile = serendipity_uploadSecure(basename($tfile));

	        if (serendipity_isActiveFile($tfile)) {
                printf(ERROR_FILE_FORBIDDEN, $tfile);
                echo '<br />';
                continue;
            }
    
            $serendipity['POST']['target_directory'][$idx] = serendipity_uploadSecure($serendipity['POST']['target_directory'][$idx], true, true);
            $target = $serendipity['serendipityPath'] . $serendipity['uploadPath'] . $serendipity['POST']['target_directory'][$idx] . $tfile;
    
            if (file_exists($target)) {
                echo '(' . $target . ') ' . ERROR_FILE_EXISTS_ALREADY;
                echo '<br />';
            } else {
                // Accept file
                if (is_uploaded_file($uploadtmp) && move_uploaded_file($uploadtmp, $target)) {
                    printf(FILE_UPLOADED . '<br />', $uploadfile, $target);
                    @umask(0000);
                    @chmod($target, 0664);
    
                    // Create thumbnail
                    if ( $created_thumbnail = serendipity_makeThumbnail($tfile, $serendipity['POST']['target_directory'][$idx]) ) {
                        echo THUMB_CREATED_DONE . '<br />';
                    }
                    // Insert into database
                    $image_id = serendipity_insertImageInDatabase($tfile, $serendipity['POST']['target_directory'][$idx], $authorid);
                    serendipity_plugin_api::hook_event('backend_image_add', $target);
                } else {
                    echo ERROR_UNKNOWN_NOUPLOAD . '<br />';
                }
            }
        }
    }
    break;


    case 'directoryDoDelete':
        if (!serendipity_checkFormToken() || !serendipity_checkPermission('adminImagesDirectories')) {
            return;
        }

        $new_dir = serendipity_uploadSecure($serendipity['GET']['dir'], true);
        if (is_dir($serendipity['serendipityPath'] . $serendipity['uploadPath'] . $new_dir)) {
            if (!is_writable($serendipity['serendipityPath'] . $serendipity['uploadPath'] . $new_dir)) {
                printf(DIRECTORY_WRITE_ERROR, $new_dir);
            } else {
                // Directory exists and is writable. Now dive within subdirectories and kill 'em all.
                serendipity_killPath($serendipity['serendipityPath'] . $serendipity['uploadPath'], $new_dir, (isset($serendipity['POST']['nuke']) ? true : false));
            }
        } else {
            printf(ERROR_NO_DIRECTORY, $new_dir);
        }

        break;

    case 'directoryDelete':
        if (!serendipity_checkPermission('adminImagesDirectories')) {
            return;
        }
?>

    <strong><?php echo DELETE_DIRECTORY ?></strong><br />
    <?php echo DELETE_DIRECTORY_DESC ?>
    <br />
    <br />
    <form method="POST" action="?serendipity[adminModule]=images&serendipity[adminAction]=directoryDoDelete&amp;serendipity[dir]=<?php echo $serendipity['GET']['dir'] ?>">
    <?php echo serendipity_setFormToken(); ?> 
    <table cellpadding="5">
        <tr>
            <td width="100"><strong><?php echo NAME ?></strong></td>
            <td><?php echo basename($serendipity['GET']['dir']) ?></td>
        </tr>
        <tr>
            <td colspan="2"><input type="checkbox" name="serendipity[nuke]" value="true" style="margin: 0"> <?php echo FORCE_DELETE ?></td>
        </tr>
    </table>
    <br />
    <br />
    <div align="center">
        <?php echo sprintf(CONFIRM_DELETE_DIRECTORY, $serendipity['GET']['dir']) ?><br />
        <input name="SAVE" value="<?php echo DELETE_DIRECTORY ?>" class="serendipityPrettyButton" type="submit">
    </div>
    </form>

<?php
        break;

    case 'directoryDoCreate':
        if (!serendipity_checkFormToken() || !serendipity_checkPermission('adminImagesDirectories')) {
            return;
        }

        $new_dir = serendipity_uploadSecure($serendipity['POST']['parent'] . '/' . $serendipity['POST']['name'], true);
        $new_dir = str_replace('..', '', $new_dir);

        /* TODO: check if directory already exist */
        if (@mkdir($serendipity['serendipityPath'] . $serendipity['uploadPath'] . $new_dir)) {
            printf(DIRECTORY_CREATED, $serendipity['POST']['name']);
            @umask(0000);
            @chmod($serendipity['serendipityPath'] . $serendipity['uploadPath'] . $new_dir, 0777);
        } else {
            printf(DIRECTORY_WRITE_ERROR, $new_dir);
        }

        break;

    case 'directoryCreate':
        if (!serendipity_checkPermission('adminImagesDirectories')) {
            return;
        }
?>
    <strong><?php echo CREATE_DIRECTORY ?></strong><br />
    <?php echo CREATE_DIRECTORY_DESC ?>
    <br />
    <br />
    <form method="POST" action="?serendipity[adminModule]=images&serendipity[adminAction]=directoryDoCreate">
    <?php echo serendipity_setFormToken(); ?> 
    <table cellpadding="5">
        <tr>
            <td><?php echo NAME ?></td>
            <td><input type="text" name="serendipity[name]" value="" /></td>
        </tr>
        <tr>
            <td><?php echo PARENT_DIRECTORY ?></td>
            <td><select name="serendipity[parent]">
                    <option value=""><?php echo BASE_DIRECTORY ?></option>
                <?php foreach ( serendipity_traversePath($serendipity['serendipityPath'] . $serendipity['uploadPath']) as $folder ) { ?>
                    <option value="<?php echo $folder['relpath'] ?>"><?php echo str_repeat('&nbsp;', $folder['depth']*2) . ' '. $folder['name'] ?></option>
                <?php } ?>
                </select>
            </td>
        </tr>
    </table>
    <div><input name="SAVE" value="<?php echo CREATE_DIRECTORY ?>" class="serendipityPrettyButton" type="submit"></div>
    </form>
<?php
        break;

    case 'directorySelect':
        if (!serendipity_checkPermission('adminImagesDirectories')) {
            return;
        }

?>
    <br />
    <?php echo DIRECTORIES_AVAILABLE; ?>
    <br />
    <table border="0" cellspacing="0" cellpadding="4" width="100%">
        <tr>
            <td colspan="2"><strong><?php echo BASE_DIRECTORY ?></strong></td>
        </tr>
        <?php foreach ( serendipity_traversePath($serendipity['serendipityPath'] . $serendipity['uploadPath']) as $folder ) { ?>
        <tr>
            <td width="16"><a href="?serendipity[adminModule]=images&amp;serendipity[adminAction]=directoryDelete&amp;serendipity[dir]=<?php echo urlencode($folder['relpath']) ?>"><img src="<?php echo serendipity_getTemplateFile('admin/img/delete.png') ?>" alt="<?php echo DELETE ?>" border="0"></a></td>
            <td style="padding-left: <?php echo $folder['depth']*10 ?>"><?php echo $folder['name'] ?></td>
        </tr>
        <?php } ?>
    </table>
    <br />
    <div><a href="?serendipity[adminModule]=images&serendipity[adminAction]=directoryCreate" class="serendipityPrettyButton"><?php echo CREATE_NEW_DIRECTORY ?></a></div>

<?php
        break;

    case 'addSelect':
        if (!serendipity_checkPermission('adminImagesAdd')) {
            return;
        }

?>
    <?php echo ADD_MEDIA_BLAHBLAH; ?>

    <script type="text/javascript">
    // Function prototype inspired by http://molily.de/javascript-nodelist
    function showNodes(n) {
        var html;
        html = '<!--nodeset--><li>';

        switch (n.nodeType) {
            case 1:
                html += 'Type is <em>' + n.nodeName + '<\/em>';
                if (n.hasChildNodes()) {
                    ausgabe += ' - childNodes: ' + n.childNodes.length;
                }
                break;

            case 3:
                var nval = n.nodeValue.replace(/</g, '&lt;').replace(/\n/g, '\\n');
                html += 'Content: <strong>' + nval + '<\/strong>';
                break;

            case 8:
                var nval = n.nodeValue.replace(/</g, '&lt;').replace(/\n/g, '\\n');
                html += 'Hidden: <em>' + nval + '<\/em>';
                break;

            default:
                html += 'Type is ' + n.nodeType + ', Content is <strong>' + n.nodeValue + '<\/strong>';
        }

        if (n.hasChildNodes()) {
            html += '\n<ol>\n';
            for (i=0; i < n.childNodes.length; i++) {
                j = n.childNodes[i];
                html += showNodes(j);
            }
            html += '</ol>\n';
        }
        html += '</li>\n';

        return html;
    }

    function getfilename(value) {
        re = /^.+[\/\\]+?(.+)$/;
        return value.replace(re, "$1");
    }

    isFileUpload = true;
    function hideForeign() {
        document.getElementById('foreign_upload').style.display = 'none';
        document.getElementById('imageurl').value = '';
        isFileUpload = false;
    }

    var fieldcount = 1;
    function addField() {
        fieldcount++;

        fields = document.getElementById('upload_template').cloneNode(true);
        fields.id = 'upload_form_' + fieldcount;
        fields.style.display = 'block';

        // Get the DOM outline be uncommenting this:
        //document.getElementById('debug').innerHTML = showNodes(fields);

        // garvin: This gets a bit weird. Opera, Mozilla and IE all have their own numbering.
        // We cannot operate on "ID" basis, since a unique ID is not yet set before instancing.
        if (fields.childNodes[0].nodeValue == null) {
            // This is Internet Explorer, it does not have a linebreak as first element.
            userfile       = fields.childNodes[0].childNodes[0].childNodes[0].childNodes[1].childNodes[0];
            targetfilename = fields.childNodes[0].childNodes[0].childNodes[2].childNodes[1].childNodes[0];
            targetdir      = fields.childNodes[0].childNodes[0].childNodes[3].childNodes[1].childNodes[0];
            columncount    = fields.childNodes[1].childNodes[0];
        } else {
            // We have a browser which has \n's as their own nodes. Don't ask me. Now let's check if it's Opera or Mozilla.
            if (fields.childNodes[1].childNodes[0].nodeValue == null) {
                // This is Opera.
                userfile       = fields.childNodes[1].childNodes[0].childNodes[0].childNodes[1].childNodes[0];
                targetfilename = fields.childNodes[1].childNodes[0].childNodes[2].childNodes[1].childNodes[0];
                targetdir      = fields.childNodes[1].childNodes[0].childNodes[3].childNodes[1].childNodes[0];
                columncount    = fields.childNodes[3].childNodes[0];
            } else if (fields.childNodes[1].childNodes[1].childNodes[0].childNodes[3] == null) {
            	// This is Safari.
                userfile       = fields.childNodes[1].childNodes[1].childNodes[0].childNodes[1].childNodes[0]; 
                targetfilename = fields.childNodes[1].childNodes[1].childNodes[2].childNodes[1].childNodes[0];
                targetdir      = fields.childNodes[1].childNodes[1].childNodes[3].childNodes[1].childNodes[0];
                columncount    = fields.childNodes[3].childNodes[0];            
            } else {
                // This is Mozilla.
                userfile       = fields.childNodes[1].childNodes[1].childNodes[0].childNodes[3].childNodes[0];
                targetfilename = fields.childNodes[1].childNodes[1].childNodes[4].childNodes[3].childNodes[0];
                targetdir      = fields.childNodes[1].childNodes[1].childNodes[6].childNodes[3].childNodes[0];
                columncount    = fields.childNodes[3].childNodes[0];
            }
        }
        
        userfile.id   = 'userfile_' + fieldcount;
        userfile.name = 'serendipity[userfile][' + fieldcount + ']';
        
        targetfilename.id   = 'target_filename_' + fieldcount;
        targetfilename.name = 'serendipity[target_filename][' + fieldcount + ']';
        
        targetdir.id   = 'target_directory_' + fieldcount;
        targetdir.name = 'serendipity[target_directory][' + fieldcount + ']';
        
        columncount.id   = 'column_count_' + fieldcount;
        columncount.name = 'serendipity[column_count][' + fieldcount + ']';

        iNode = document.getElementById('upload_form');
        iNode.parentNode.insertBefore(fields, iNode);
    }
    
    var inputStorage = new Array();
    function checkInputs() {
        for (i = 1; i <= fieldcount; i++) {
            if (!inputStorage[i]) {
                fillInput(i, i);
            } else if (inputStorage[i] == document.getElementById('target_filename_' + i).value) {
                fillInput(i, i);
            }
        }

    }
    
    function debugFields() {
        for (i = 1; i <= fieldcount; i++) {
            debugField('target_filename_' + i);
            debugField('userfile_' + i);
        }
    }
    
    function debugField(id) {
        alert(id + ': ' + document.getElementById(id).value);
    }

    function fillInput(source, target) {
        useDuplicate = false;

        // First field is a special value for foreign URLs instead of uploaded files
        if (source == 1 && document.getElementById('imageurl').value != "") {
            sourceval = getfilename(document.getElementById('imageurl').value);
            useDuplicate = true;
        } else {
            sourceval = getfilename(document.getElementById('userfile_' + source).value);
        } 

        if (sourceval.length > 0) {
            document.getElementById('target_filename_' + target).value = sourceval;
            inputStorage[target] = sourceval;
        }

        // Display filename in duplicate form as well!
        if (useDuplicate) {
            tkey = target + 1;
            
            if (!inputStorage[tkey] || inputStorage[tkey] == document.getElementById('target_filename_' + tkey).value) {
                document.getElementById('target_filename_' + (target+1)).value = sourceval;
                inputStorage[target + 1] = '~~~';
            }
        }
    }
    </script>

    <form action="?" method="POST" id="uploadform" enctype="multipart/form-data">
        <div>
            <?php echo serendipity_setFormToken(); ?> 
            <input type="hidden" name="serendipity[action]"      value="admin" />
            <input type="hidden" name="serendipity[adminModule]" value="images" />
            <input type="hidden" name="serendipity[adminAction]" value="add" />
<?php
        if (isset($image_selector_addvars) && is_array($image_selector_addvars)) {
            // These variables may come frmo serendipity_admin_image_selector.php to show embedded upload form
            foreach($image_selector_addvars AS $imgsel_key => $imgsel_val) {
                echo '          <input type="hidden" name="serendipity[' . htmlspecialchars($imgsel_key) . ']" value="' . htmlspecialchars($imgsel_val) . '" />' . "\n";
            }
        }
?>
            <table id="foreign_upload">
                <tr>
                    <td nowrap="nowrap"><?php echo ENTER_MEDIA_URL; ?></td>
                    <td><input type="text" id="imageurl" name="serendipity[imageurl]" 
                             onchange="checkInputs()" 
                              value="" 
                               size="40" /></td>
                </tr>
                <tr>
                    <td nowrap="nowrap"><?php echo ENTER_MEDIA_URL_METHOD; ?></td>
                    <td>
                        <select name="serendipity[imageimporttype]">
                            <option value="image"><?php echo FETCH_METHOD_IMAGE; ?></option>
                            <option value="hotlink"><?php echo FETCH_METHOD_HOTLINK; ?></option>
                        </select>
                    </td>
                </tr>

                <tr>
                    <td align="center" colspan="2"><b> - <?php echo WORD_OR; ?> - </b></td>
                </tr>
            </table>

            <!-- WARNING: Do not change spacing or breaks below. If you do, the JavaScript childNodes need to be edited. Newlines count as nodes! -->
            <div id="upload_template">
            <table style="margin-top: 35px" id="upload_table">
                <tr>
                    <td nowrap='nowrap'><?php echo ENTER_MEDIA_UPLOAD; ?></td>
                    <td><input id="userfile_1" name="serendipity[userfile][1]" 
                             onchange="checkInputs();" 
                               type="file" /></td>
                </tr>

                <tr>
                    <td align="center" colspan="2"><br /></td>
                </tr>

                <tr>
                    <td><?php echo SAVE_FILE_AS; ?></td>
                    <td><input type="text" id="target_filename_1" name="serendipity[target_filename][1]" value="" size="40" /></td>
                </tr>

                <tr>
                    <td><?php echo STORE_IN_DIRECTORY; ?></td>
                    <td><select id="target_directory_1" name="serendipity[target_directory][1]">
                        <option value=""><?php echo BASE_DIRECTORY; ?></option>
                        <?php foreach (serendipity_traversePath($serendipity['serendipityPath'] . $serendipity['uploadPath']) as $folder) { ?>
                        <option <?php echo ($serendipity['GET']['only_path'] == $folder['relpath']) ? 'selected="selected"' : '' ?> value="<?php echo $folder['relpath'] ?>"><?php echo str_repeat('&nbsp;', $folder['depth']*2) . ' '. $folder['name'] ?></option>
                        <?php } ?>
                        </select>
                    </td>
                </tr>
           </table>
           <div id="ccounter"><input type="hidden" name="serendipity[column_count][1]" id="column_count_1" value="true" /></div>
           </div>
           
           <div id="debug">
           </div>

           <script type="text/javascript">
                document.getElementById('upload_template').style.display  = 'none';
                document.write('<span id="upload_form"><' + '/span>');
                addField();
           </script>
           
            <?php serendipity_plugin_api::hook_event('backend_image_addform', $serendipity); ?>

            <div style="text-align: center; margin-top: 15px; margin-bottom: 15px">
                <script type="text/javascript">
                    document.write('<input class="serendipityPrettyButton" type="button" value="<?php echo IMAGE_MORE_INPUT; ?>" onclick="hideForeign(); addField()"' + '/><br' + '/>');
                </script>
                <input type="checkbox" name="serendipity[all_authors]" value="true" checked="checked" id="all_authors" /><label for="all_authors"><?php echo ALL_AUTHORS; ?></label> <input onclick="checkInputs();" type="submit" value="<?php echo GO; ?>" class="serendipityPrettyButton" />
            </div>
        </div>
        <div><?php echo ADD_MEDIA_BLAHBLAH_NOTE; ?></div>
    </form>
<?php
    break;

    case 'rotateCW':
        $file = serendipity_fetchImageFromDatabase($serendipity['GET']['fid']);
        if (!serendipity_checkPermission('adminImagesDelete') || (!serendipity_checkPermission('adminImagesMaintainOthers') && $file['authorid'] != '0' && $file['authorid'] != $serendipity['authorid'])) {
            return;
        }

        if (serendipity_rotateImg($serendipity['GET']['fid'], -90)) {
?>
        <script language="javascript" type="text/javascript">
            location.href="<?php echo htmlspecialchars($_SERVER['HTTP_REFERER']) ?>";
        </script>
	<noscript><a href="<?php echo htmlspecialchars($_SERVER['HTTP_REFERER']) ?>"><?php echo DONE ?></a></noscript>
<?php
        }
    break;

    case 'rotateCCW':
        $file = serendipity_fetchImageFromDatabase($serendipity['GET']['fid']);
        if (!serendipity_checkPermission('adminImagesDelete') || (!serendipity_checkPermission('adminImagesMaintainOthers') && $file['authorid'] != '0' && $file['authorid'] != $serendipity['authorid'])) {
            return;
        }

        if (serendipity_rotateImg($serendipity['GET']['fid'], 90)) {
?>
        <script language="javascript" type="text/javascript">
            location.href="<?php echo htmlspecialchars($_SERVER['HTTP_REFERER']) ?>";
        </script>
	<noscript><a href="<?php echo htmlspecialchars($_SERVER['HTTP_REFERER']) ?>"><?php echo DONE ?></a></noscript>
<?php
        }
    break;

    case 'scale':
        $file = serendipity_fetchImageFromDatabase($serendipity['GET']['fid']);

        if (!serendipity_checkFormToken() || !serendipity_checkPermission('adminImagesDelete') || (!serendipity_checkPermission('adminImagesMaintainOthers') && $file['authorid'] != '0' && $file['authorid'] != $serendipity['authorid'])) {
            return;
        }

        printf(
          SCALING_IMAGE . '<br />',

          $file['path'] . $file['name'] .'.'. $file['extension'],
          $serendipity['GET']['width'],
          $serendipity['GET']['height']
        );

        echo serendipity_scaleImg($serendipity['GET']['fid'], $serendipity['GET']['width'], $serendipity['GET']['height']) . '<br />';
        echo DONE . '<br />';
        // Forward user to overview (we don't want the user's back button to rename things again)
?>
    <script language="javascript" type="text/javascript">
       // location.href="?serendipity[adminModule]=images";
    </script>
    <noscript><a href="<?php echo htmlspecialchars($_SERVER['HTTP_REFERER']) ?>"><?php echo DONE ?></a></noscript>
<?php
        break;

    case 'scaleSelect':
        $file = serendipity_fetchImageFromDatabase($serendipity['GET']['fid']);

        if (!serendipity_checkPermission('adminImagesDelete') || (!serendipity_checkPermission('adminImagesMaintainOthers') && $file['authorid'] != '0' && $file['authorid'] != $serendipity['authorid'])) {
            return;
        }

        $s = getimagesize($serendipity['serendipityPath'] . $serendipity['uploadPath'] . $file['path'] . $file['name'] .'.'. $file['extension']);
?>
    <script type="text/javascript" language="javascript">
    <!--
        function rescale(dim, newval) {
            var originalWidth  = <?php echo $s[0]; ?>;
            var originalHeight = <?php echo $s[1]; ?>;
            var ratio          = originalHeight/originalWidth;
            var trans          = new Array();
            trans['width']     = new Array('serendipity[height]', ratio);
            trans['height']    = new Array('serendipity[width]', 1/ratio);

            if (document.serendipityScaleForm.elements['auto'].checked == true) {
                document.serendipityScaleForm.elements[trans[dim][0]].value=Math.round(trans[dim][1]*newval);
            }

            document.getElementsByName('serendipityScaleImg')[0].style.width =
              document.serendipityScaleForm.elements['serendipity[width]'].value+'px';

            document.getElementsByName('serendipityScaleImg')[0].style.height =
              document.serendipityScaleForm.elements['serendipity[height]'].value+'px';
        }
    //-->
    </script>
<?php

        printf(RESIZE_BLAHBLAH, $serendipity['GET']['fname']);
        printf(ORIGINAL_SIZE, $s[0],$s[1]);
        echo HERE_YOU_CAN_ENTER_BLAHBLAH;
?>
    <form name="serendipityScaleForm" action="?" method="GET">
        <div>
            <?php echo NEWSIZE; ?>

            <?php echo serendipity_setFormToken(); ?> 
            <input type="hidden" name="serendipity[adminModule]" value="images" />
            <input type="hidden" name="serendipity[adminAction]" value="scale" />
            <input type="hidden" name="serendipity[fid]"         value="<?php echo $serendipity["GET"]["fid"]; ?>" />

            <input type="text" size="4" name="serendipity[width]"   onchange="rescale('width' , value);" value="<?php echo $s[0]; ?>" />x
            <input type="text" size="4" name="serendipity[height]"  onchange="rescale('height', value);" value="<?php echo $s[1]; ?>" />
            <br />

            <?php echo KEEP_PROPORTIONS; ?>:
            <!-- <input type='button' value='preview'>-->
            <input type="checkbox" name="auto"  checked="checked" /><br />
            <input type="button"   name="scale" value="<?php echo IMAGE_RESIZE; ?>" onclick="if (confirm('<?php echo REALLY_SCALE_IMAGE; ?>')) document.serendipityScaleForm.submit();" class="serendipityPrettyButton" />
        </div>
    </form>

    <img src="<?php echo $serendipity['uploadHTTPPath'] . $file['path'] . $file['name'] .'.'. $file['extension'] ; ?>" name="serendipityScaleImg" style="width: <?php echo $s[0]; ?>px; height: <?php echo $s[1]; ?>px;" alt="" />
<?php
        break;

    default:
        if (!serendipity_checkPermission('adminImagesView')) {
            return;
        }

?>
<script type="text/javascript" language="javascript">
    <!--
        function rename(id, fname) {
            if(newname = prompt('<?php echo ENTER_NEW_NAME; ?>' + fname, fname)) {
                location.href='?<?php echo serendipity_setFormToken('url'); ?>&serendipity[adminModule]=images&serendipity[adminAction]=rename&serendipity[fid]='+ escape(id) + '&serendipity[newname]='+ escape(newname);
            }
        }
    //-->
    </script>


<?php
        serendipity_displayImageList(
          isset($serendipity['GET']['page'])   ? $serendipity['GET']['page']   : 1,
          2,
          true
        );

        break;
}
/* vim: set sts=4 ts=4 expandtab : */
