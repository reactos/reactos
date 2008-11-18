<?php # $Id: serendipity_admin_image_selector.php 487 2005-09-23 13:47:58Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

include('serendipity_config.inc.php');

if (IN_serendipity !== true) {
    die ("Don't hack!");
}

header('Content-Type: text/html; charset=' . LANG_CHARSET);

if ($_SESSION['serendipityAuthedUser'] !== true)  {
    die(HAVE_TO_BE_LOGGED_ON);
}

if (!isset($serendipity['GET']['adminModule'])) {
    $serendipity['GET']['adminModule'] = (isset($serendipity['POST']['adminModule']) ? $serendipity['POST']['adminModule'] : '');
}

if (!isset($serendipity['GET']['step'])) {
    $serendipity['GET']['step']        = (isset($serendipity['POST']['step'])        ? $serendipity['POST']['step']        : '');
}

function ifRemember($name, $value, $isDefault = false, $att = 'checked') {
    global $serendipity;
    
    if (!is_array($serendipity['COOKIE']) && !$isDefault) {
        return false;
    }
    
    if ((!is_array($serendipity['COOKIE']) && $isDefault) || 
        (!isset($serendipity['COOKIE']['serendipity_' . $name]) && $isDefault) ||
        (isset($serendipity['COOKIE']['serendipity_' . $name]) && $serendipity['COOKIE']['serendipity_' . $name] == $value)) {

        return " $att=\"$att\" ";
    }
}
?>
<html>
    <head>
        <title><?php echo SELECT_FILE; ?></title>
        <meta http-equiv="Content-Type" content="text/html; charset=<?php echo LANG_CHARSET; ?>" />
        <link rel="stylesheet" type="text/css" href="<?php echo serendipity_rewriteURL('serendipity_admin.css'); ?>" />
    </head>

    <script type="text/javascript">
        function SetCookie(name, value) {
            var today  = new Date();
            var expire = new Date();
            expire.setTime(today.getTime() + (60*60*24*30));
            document.cookie = 'serendipity[' + name + ']='+escape(value) + ';expires=' + expire.toGMTString();
        }

        function rememberOptions() {
            el = document.getElementById('imageForm');
            for (i = 0; i < el.elements.length; i++) {
                elname = new String(el.elements[i].name);
                elname = elname.replace(/\[/g, '_');
                elname = elname.replace(/\]/g, '');
                
                if (el.elements[i].type == 'radio') {
                    if (el.elements[i].checked) {
                        SetCookie(elname, el.elements[i].value);
                    }
                } else if (typeof(el.elements[i].options) == 'object') {
                    SetCookie(elname, el.elements[i].options[el.elements[i].selectedIndex].value);
                }
            }
        }
    </script>
<body id="serendipityAdminBodyImageSelector">

<div class="serendipityAdminContent">
<?php
switch ($serendipity['GET']['step']) {
    case '1':
        if (isset($serendipity['GET']['adminAction'])) { // Embedded upload form
            switch ($serendipity['GET']['adminAction']) {
                case 'addSelect':
                    $image_selector_addvars = array(
                        'step'          => 1,
                        'textarea'      => (!empty($serendipity['GET']['textarea'])      ? $serendipity['GET']['textarea']      : ''),
                        'htmltarget'    => (!empty($serendipity['GET']['htmltarget'])    ? $serendipity['GET']['htmltarget']    : ''),
                        'filename_only' => (!empty($serendipity['GET']['filename_only']) ? $serendipity['GET']['filename_only'] : '')
                    );
                    include S9Y_INCLUDE_PATH . 'include/admin/images.inc.php';
                    break 2;

                case 'add':
                    include S9Y_INCLUDE_PATH . 'include/admin/images.inc.php';
                    if (isset($created_thumbnail) && is_array($created_thumbnail)) {
                        $serendipity['GET']['image'] = (int)$image_id; // $image_id is passed from images.inc.php

                        if (!empty($serendipity['POST']['htmltarget'])) {
                            $serendipity['GET']['htmltarget'] = $serendipity['POST']['htmltarget'];
                        }

                        if (!empty($serendipity['POST']['filename_only'])) {
                            $serendipity['GET']['filename_only'] = $serendipity['POST']['filename_only'];
                        }

                        if (!empty($serendipity['POST']['textarea'])) {
                            $serendipity['GET']['textarea'] = $serendipity['POST']['textarea'];
                        }
                        break;
                    } else {
                        break 2;
                    }
            }
        }

        $file           = serendipity_fetchImageFromDatabase($serendipity['GET']['image']);
        $file['imgsrc'] = $serendipity['serendipityHTTPPath'] . $serendipity['uploadHTTPPath'] . $file['path'] . $file['name'] . (!empty($file['thumbnail_name']) ? '.' . $file['thumbnail_name'] : '') . '.' . $file['extension'];
        if ($file['hotlink']) {
            $imgName    = $file['path'];
        } else {
            $imgName    = $serendipity['serendipityHTTPPath'] . $serendipity['uploadHTTPPath'] . $file['path'] . $file['name'] .'.'. $file['extension'];
        }

        $thumbbasename  = $file['path'] . $file['name'] . (!empty($file['thumbnail_name']) ? '.' . $file['thumbnail_name'] : '') . '.' . $file['extension'];

        if ($file['hotlink']) {
            $thumbName  = $file['path'];
        } else {
            $thumbName  = $serendipity['serendipityHTTPPath'] . $serendipity['uploadHTTPPath'] . $thumbbasename;
        }
        $thumbsize     = @getimagesize($serendipity['serendipityPath'] . $serendipity['uploadPath'] . $thumbbasename);
        $is_image      = serendipity_isImage($file, true);
?>
<script type="text/javascript" language="JavaScript" src="<?php echo $serendipity['serendipityHTTPPath']; ?>serendipity_define.js.php"></script>
<script type="text/javascript" language="Javascript" src="<?php echo $serendipity['serendipityHTTPPath']; ?>serendipity_editor.js"></script>

<div>
<?php
        if (!file_exists($file['imgsrc']) && $is_image) {

            $dimWidth  = $file['dimensions_width'];
            $dimHeight = $file['dimensions_height'];

            if ($file['hotlink']) {
                $thumbDim    = @serendipity_calculate_aspect_size($dimWidth, $dimHeight, $serendipity['thumbSize']);
                $thumbWidth  = $thumbDim[0];
                $thumbHeight = $thumbDim[1];
                $imgsrc      = $file['path'];
            } else {
                $thumbWidth  = $thumbsize[0];
                $thumbHeight = $thumbsize[1];
                $imgsrc      = $file['imgsrc'];
            }
            $file['finishJSFunction'] = 'serendipity_imageSelector_done(\'' . $serendipity['GET']['textarea'] . '\')';
            serendipity_plugin_api::hook_event('frontend_image_selector', $file);
?>
    <img align="right" src="<?php echo $imgsrc; ?>">
    <h1><?php printf(YOU_CHOSE, $file['name']); ?></h1>
    <p>
        <form action="#" method="GET" id="imageForm" name="serendipity[selForm]" onsubmit="serendipity_imageSelector_done()">
            <div>
                <?php serendipity_plugin_api::hook_event('frontend_image_selector_hiddenfields', $file); ?>
                <input type="hidden" name="imgThumbWidth"  value="<?php echo $thumbWidth; ?>" />
                <input type="hidden" name="imgThumbHeight" value="<?php echo $thumbHeight; ?>" />
                <input type="hidden" name="imgWidth"  value="<?php echo $dimWidth; ?>" />
                <input type="hidden" name="imgHeight" value="<?php echo $dimHeight; ?>" />
                <input type="hidden" name="imgName"   value="<?php echo $imgName; ?>" />
                <input type="hidden" name="thumbName" value="<?php echo $thumbName; ?>" />
                <input type="hidden" name="hotlink" value="<?php echo $file['hotlink']; ?>" />
                <?php if (!empty($serendipity['GET']['htmltarget'])) { ?>
                <input type="hidden" name="serendipity[htmltarget]" value="<?php echo htmlspecialchars($serendipity['GET']['htmltarget']); ?>" />
                <?php } ?>
                <?php if (!empty($serendipity['GET']['filename_only'])) { ?>
                <input type="hidden" name="serendipity[filename_only]" value="<?php echo htmlspecialchars($serendipity['GET']['filename_only']); ?>" />
                <?php } ?>

                <b><?php echo IMAGE_SIZE; ?>:</b>
                <br />
                <input id="radio_link_no" type="radio" name="serendipity[linkThumbnail]" value="no" <?php echo ifRemember('linkThumbnail', 'no', true); ?> /><label for="radio_link_no"><?php echo I_WANT_THUMB; ?></label><br />
                <input id="radio_link_yes" type="radio" name="serendipity[linkThumbnail]" value="yes" <?php echo ifRemember('linkThumbnail', 'yes'); ?> /><label for="radio_link_yes"><?php echo I_WANT_BIG_IMAGE; ?></label><br />
                <?php serendipity_plugin_api::hook_event('frontend_image_selector_imagesize', $file); ?>
                <br />

                <?php if (empty($serendipity['GET']['filename_only']) || $serendipity['GET']['filename_only'] != 'true') { ?>
                <b><?php echo IMAGE_ALIGNMENT; ?>:</b>
                <br />
                <input type="radio" name="serendipity[align]" <?php echo ifRemember('align', ''); ?>      value="" />                       <img src="<?php echo serendipity_getTemplateFile('img/img_align_top.png') ?>"   vspace="5" /><br />
                <input type="radio" name="serendipity[align]" <?php echo ifRemember('align', 'left', true); ?>  value="left" /> <img src="<?php echo serendipity_getTemplateFile('img/img_align_left.png') ?>"  vspace="5" /><br />
                <input type="radio" name="serendipity[align]" <?php echo ifRemember('align', 'right'); ?> value="right" />                  <img src="<?php echo serendipity_getTemplateFile('img/img_align_right.png') ?>" vspace="5" /><br />
                <?php serendipity_plugin_api::hook_event('frontend_image_selector_imagealign', $file); ?>
                <br />

                <b><?php echo IMAGE_AS_A_LINK; ?>:</b>
                <br />
                <input id="radio_islink_yes" type="radio" name="serendipity[isLink]" value="yes" <?php echo ifRemember('isLink', 'yes', true); ?> /><label for="radio_islink_yes"> <?php echo I_WANT_NO_LINK; ?></label><br />
                <input id="radio_islink_no"  type="radio" name="serendipity[isLink]" value="no" <?php  echo ifRemember('isLink', 'no'); ?> /><label for="radio_islink_no"> <?php echo I_WANT_IT_TO_LINK; ?></label>
                <?php if ($file['hotlink']) { ?>
                <input type="text"  name="serendipity[url]" size="30" value="<?php echo $file['path']; ?>" /><br />
                <?php } else { ?>
                <input type="text"  name="serendipity[url]" size="30" value="<?php echo $serendipity['serendipityHTTPPath'] . $serendipity['uploadHTTPPath'] . $file['path'] . $file['name'] .'.'. $file['extension']; ?>" /><br />
                <?php } ?>
                <?php serendipity_plugin_api::hook_event('frontend_image_selector_imagelink', $file); ?>
                <br />

                <b><?php echo COMMENT; ?>:</b>
                <br />
                <textarea id="serendipity_imagecomment" name="serendipity[imagecomment]" rows="5" cols="40"></textarea>
                <?php serendipity_plugin_api::hook_event('frontend_image_selector_imagecomment', $file); ?>
                <br />
                <?php } ?>

                <?php serendipity_plugin_api::hook_event('frontend_image_selector_more', $file); ?>

                <input type="button" value="<?php echo BACK; ?>" onclick="history.go(-1);" />
                <input type="button" value="<?php echo DONE; ?>" onclick="rememberOptions(); <?php echo $file['finishJSFunction']; ?>" />
                <?php serendipity_plugin_api::hook_event('frontend_image_selector_submit', $file); ?>
            </div>
        </form>
    </p>
<?php
    } else {
        // What to do if file is no image.
        // For the future, maybe allow the user to add title/link description and target window

        if (!empty($serendipity['GET']['filename_only'])) {
?>
    <script type="text/javascript">
        <?php serendipity_plugin_api::hook_event('frontend_image_add_filenameonly', $serendipity); ?>
        self.opener.serendipity_imageSelector_addToElement('<?php echo htmlspecialchars($imgName); ?>', '<?php echo htmlspecialchars($serendipity['GET']['htmltarget']); ?>');
        self.close();
    </script>
<?php
        } else {
?>
    <script type="text/javascript">
    block = '<a href="<?php echo htmlspecialchars($imgName); ?>" title="<?php echo htmlspecialchars($file['name'] . '.' . $file['extension']); ?>" target="_blank"><?php echo htmlspecialchars($file['name'] . '.' . $file['extension']); ?></a>';
    <?php serendipity_plugin_api::hook_event('frontend_image_add_unknown', $serendipity); ?>
    if (self.opener.editorref) {
        self.opener.editorref.surroundHTML(block, '');
    } else {
        self.opener.serendipity_imageSelector_addToBody(block, '<?php echo $serendipity['GET']['textarea']; ?>');
    }
    self.close();
    </script>
<?php
        }
    }
    break;

    default:
        $add_url = '';
        if (!empty($serendipity['GET']['htmltarget'])) {
            $add_url .= '&amp;serendipity[htmltarget]=' . $serendipity['GET']['htmltarget'];
        }

        if (!empty($serendipity['GET']['filename_only'])) {
            $add_url .= '&amp;serendipity[filename_only]=' . $serendipity['GET']['filename_only'];
        }
?>
    <h1><?php echo SELECT_FILE; ?></h1>
    <h2><?php echo CLICK_FILE_TO_INSERT; ?></h2>
    <br />

    <?php
        serendipity_displayImageList(
          isset($serendipity['GET']['page'])   ? $serendipity['GET']['page']   : 1,
          3,
          false,
          '?serendipity[step]=1' . $add_url . '&amp;serendipity[textarea]='. $serendipity['GET']['textarea'],
          true
        );
}
?>

</div>
</body>
</html>

<?php
/* vim: set sts=4 ts=4 expandtab : */
