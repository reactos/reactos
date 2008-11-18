<?php # $Id: category.inc.php 499 2005-09-27 11:40:14Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

if (IN_serendipity !== true) {
    die ("Don't hack!");
}

if (!serendipity_checkPermission('adminCategories')) {
    return;
}

$admin_category = (!serendipity_checkPermission('adminCategoriesMaintainOthers') ? "AND (authorid = 0 OR authorid = " . (int)$serendipity['authorid'] . ")" : '');

/* Add a new category */
if (isset($_POST['SAVE']) && serendipity_checkFormToken()) {
    $name     = $serendipity['POST']['cat']['name'];
    $desc     = $serendipity['POST']['cat']['description'];

    if (is_array($serendipity['POST']['cat']['write_authors']) && in_array(0, $serendipity['POST']['cat']['write_authors'])) {
        $authorid = 0;
    } else {
        $authorid = $serendipity['authorid'];
    }

    $icon     = $serendipity['POST']['cat']['icon'];
    $parentid = (isset($serendipity['POST']['cat']['parent_cat']) && is_numeric($serendipity['POST']['cat']['parent_cat'])) ? $serendipity['POST']['cat']['parent_cat'] : 0;

    if ($serendipity['GET']['adminAction'] == 'new') {
        if ($parentid != 0) {
            // TODO: This doesn't seem to work as expected, serendipity_rebuildCategoryTree(); is still needed! Only activate this optimization function when it's really working :)
            // TODO: This works if only one subcategory exists.  Otherwise, the first query will return an array.
            /*
            $res = serendipity_db_query("SELECT category_right FROM {$serendipity['dbPrefix']}category WHERE parentid={$parentid}");
            serendipity_db_query("UPDATE {$serendipity['dbPrefix']}category SET category_left=category_left+2, category_right=category_right+2 WHERE category_right>{$res}");
            */
        }

        /* Check to see if a category with the same name, already exist */
        $sql = serendipity_db_query("SELECT * FROM {$serendipity['dbPrefix']}category
                                        WHERE category_name = '". serendipity_db_escape_string($name) ."'", true);
        if ( $sql ) {
            echo '<div class="serendipityAdminMsgError">'. sprintf(CATEGORY_ALREADY_EXIST, htmlspecialchars($name)) .'</div>';
        } else {
            $catid = serendipity_addCategory($name, $desc, $authorid, $icon, $parentid);
            serendipity_ACLGrant($catid, 'category', 'read', $serendipity['POST']['cat']['read_authors']);
            serendipity_ACLGrant($catid, 'category', 'write', $serendipity['POST']['cat']['write_authors']);

            echo '<div class="serendipityAdminMsgSuccess">'. CATEGORY_SAVED .'</div>';
        }


    } elseif ($serendipity['GET']['adminAction'] == 'edit') {
            /* Check to see if a category with the same name, already exist */
            $sql = serendipity_db_query("SELECT * FROM {$serendipity['dbPrefix']}category
                                                WHERE category_name = '". serendipity_db_escape_string($name) ."'
                                                    AND categoryid <> ". (int)$serendipity['GET']['cid'], true);
            if ( $sql ) {
                echo '<div class="serendipityAdminMsgError">'. sprintf(CATEGORY_ALREADY_EXIST, htmlspecialchars($name)) .'</div>';
            } else if (!serendipity_checkPermission('adminCategoriesMaintainOthers') && !serendipity_ACLCheck($serendipity['authorid'], $serendipity['GET']['cid'], 'category', 'write')) {
                echo '<div class="serendipityAdminMsgError">'. PERM_DENIED .'</div>';
            } else {
                /* Check to make sure parent is not a child of self */
                $r = serendipity_db_query("SELECT categoryid FROM {$serendipity['dbPrefix']}category c
                                                WHERE c.categoryid = ". (int)$parentid ."
                                                    AND c.category_left BETWEEN " . implode(' AND ', serendipity_fetchCategoryRange((int)$serendipity['GET']['cid'])));
                if ( is_array($r) ) {
                    $r = serendipity_db_query("SELECT category_name FROM {$serendipity['dbPrefix']}category
                                                        WHERE categoryid = ". (int)$parentid);
                    echo sprintf(ALREADY_SUBCATEGORY, htmlspecialchars($r[0]['category_name']), htmlspecialchars($name));
                } else {
                    serendipity_updateCategory($serendipity['GET']['cid'], $name, $desc, $authorid, $icon, $parentid);
                    serendipity_ACLGrant($serendipity['GET']['cid'], 'category', 'read', $serendipity['POST']['cat']['read_authors']);
                    serendipity_ACLGrant($serendipity['GET']['cid'], 'category', 'write', $serendipity['POST']['cat']['read_authors']);
                    echo '<div class="serendipityAdminMsgSuccess">'. CATEGORY_SAVED .'</div>';
                }
            }
    }

    serendipity_rebuildCategoryTree();
    $serendipity['GET']['adminAction'] = 'view';
}

/* Delete a category */
if ($serendipity['GET']['adminAction'] == 'doDelete' && serendipity_checkFormToken()) {
    if ($serendipity['GET']['cid'] != 0) {
        $remaining_cat = (int)$serendipity['POST']['cat']['remaining_catid'];
        $category_ranges = serendipity_fetchCategoryRange((int)$serendipity['GET']['cid']);
        $category_range  = implode(' AND ', $category_ranges);
        if ($serendipity['dbType'] == 'postgres' || $serendipity['dbType'] == 'sqlite') {
            $query = "UPDATE {$serendipity['dbPrefix']}entrycat
                        SET categoryid={$remaining_cat} WHERE entryid IN
                        (
                          SELECT DISTINCT(e.id) FROM {$serendipity['dbPrefix']}entries e,
                          {$serendipity['dbPrefix']}category c,
                          {$serendipity['dbPrefix']}entrycat ec
                          WHERE e.id=ec.entryid AND c.categoryid=ec.categoryid
                          AND c.category_left BETWEEN {$category_range} {$admin_category}
                        )";
        } else {
            $query = "UPDATE {$serendipity['dbPrefix']}entries e,
                        {$serendipity['dbPrefix']}entrycat ec,
                        {$serendipity['dbPrefix']}category c
                      SET ec.categoryid={$remaining_cat}
                        WHERE e.id = ec.entryid
                          AND c.categoryid = ec.categoryid
                          AND c.category_left BETWEEN {$category_range}
                          {$admin_category}";
        }
        if ( serendipity_db_query($query) ) {
            if (serendipity_deleteCategory($category_range, $admin_category) ) {

                foreach($category_ranges AS $cid) {
                    if (serendipity_ACLCheck($serendipity['authorid'], $cid, 'category', 'write')) {
                        serendipity_ACLGrant($cid, 'category', 'read', array());
                        serendipity_ACLGrant($cid, 'category', 'write', array());
                    }
                }

                echo '<div class="serendipityAdminMsgSuccess">'. ($remaining_cat ? sprintf(CATEGORY_DELETED_ARTICLES_MOVED, (int)$serendipity['GET']['cid'], $remaining_cat) : sprintf(CATEGORY_DELETED,(int)$serendipity['GET']['cid'])) .'</div>';
                $serendipity['GET']['adminAction'] = 'view';
            }
        }
    } else {
        echo '<div class="serendipityAdminMsgError">'. INVALID_CATEGORY .'</div>';
    }
}
?>

<?php
    if ( $serendipity['GET']['adminAction'] == 'delete' ) {
        $this_cat = serendipity_fetchCategoryInfo($serendipity['GET']['cid']);
        if (   (serendipity_checkPermission('adminCategoriesDelete') && serendipity_checkPermission('adminCategoriesMaintainOthers'))
            || (serendipity_checkPermission('adminCategoriesDelete') && ($serendipity['authorid'] == $this_cat['authorid'] || $this_cat['authorid'] == '0')) 
            || (serendipity_checkPermission('adminCategoriesDelete') && serendipity_ACLCheck($serendipity['authorid'], $serendipity['GET']['cid'], 'category', 'write'))) { 
?>
        <form method="POST" name="serendipityCategory" action="?serendipity[adminModule]=category&amp;serendipity[adminAction]=doDelete&amp;serendipity[cid]=<?php echo $serendipity['GET']['cid'] ?>">
        <?php echo serendipity_setFormToken(); ?>
            <br />
            <?php echo CATEGORY_REMAINING ?>:
            <select name="serendipity[cat][remaining_catid]">
                <option value="0">- <?php echo NO_CATEGORY ?> -</option>
<?php
    $cats = serendipity_fetchCategories('all');
    /* TODO, show dropdown as nested categories */
    foreach ($cats as $cat_data) {
        if ($cat_data['categoryid'] != $serendipity['GET']['cid'] && (serendipity_checkPermission('adminCategoriesMaintainOthers') || $cat_data['authorid'] == '0' || $cat_data['authorid'] == $serendipity['authorid'])) {
            echo '<option value="' . $cat_data['categoryid'] . '">' . htmlspecialchars($cat_data['category_name']) . '</option>' . "\n";
        }
    }
?>
            </select>
            <input type="submit" name="REMOVE" value="<?php echo GO ?>" class="serendipityPrettyButton">
        </form>
<?php
        }
    }
?>



<?php if ( $serendipity['GET']['adminAction'] == 'edit' || $serendipity['GET']['adminAction'] == 'new' ) {
        if ( $serendipity['GET']['adminAction'] == 'edit' ) {
            $cid = (int)$serendipity['GET']['cid'];
            $this_cat = serendipity_fetchCategoryInfo($cid);
            echo '<strong>'. sprintf(EDIT_THIS_CAT, htmlspecialchars($this_cat['category_name'])) .'</strong>';
            $save = SAVE;
            $read_groups  = serendipity_ACLGet($cid, 'category', 'read');
            $write_groups = serendipity_ACLGet($cid, 'category', 'write');
        } else {
            $cid = false;
            $this_cat = array();
            echo '<strong>'. CREATE_NEW_CAT .'</strong>';
            $save = CREATE;
            $read_groups  = array(0 => 0);
            $write_groups = array(0 => 0);
        }

        $groups = serendipity_getAllGroups();
?>
<form method="POST" name="serendipityCategory">
<?php echo serendipity_setFormToken(); ?>
<table cellpadding="5" width="100%">
    <tr>
        <td><?php echo NAME; ?></td>
        <td><input type="text" name="serendipity[cat][name]" value="<?php echo isset($this_cat['category_name']) ? htmlspecialchars($this_cat['category_name']) : ''; ?>" /></td>
        <td rowspan="5" align="center" valign="middle" width="200" style="border: 1px solid #ccc"><img src="<?php echo isset($this_cat['category_icon']) ? $this_cat['category_icon'] : '' ?>" id="imagepreview" <?php echo empty($this_cat['category_icon']) ? 'style="display: none"' : '' ?>></td>
    </tr>

    <tr>
        <td><?php echo DESCRIPTION; ?></td>
        <td><input type="text" name="serendipity[cat][description]" value="<?php echo isset($this_cat['category_description']) ? htmlspecialchars($this_cat['category_description']) : ''; ?>" /></td>
    </tr>

    <tr>
        <td><?php echo IMAGE; ?></td>
        <td>
            <script type="text/javascript" language="JavaScript" src="serendipity_editor.js"></script>
            <input type="text" id="img_icon" name="serendipity[cat][icon]" value="<?php echo isset($this_cat['category_icon']) ? htmlspecialchars($this_cat['category_icon']) : ''; ?>" onchange="document.getElementById('imagepreview').src = this.value; document.getElementById('imagepreview').style.display = '';" />
            <script type="text/javascript" language="JavaScript">document.write('<input type="button" name="insImage" value="<?php echo IMAGE ; ?>" onclick="window.open(\'serendipity_admin_image_selector.php?serendipity[htmltarget]=img_icon&amp;serendipity[filename_only]=true\', \'ImageSel\', \'width=800,height=600,toolbar=no,scrollbars=1,scrollbars,resize=1,resizable=1\');" class="serendipityPrettyButton" />');</script><!-- noscript>FIXXME: Emit a warning if JS is disabled</noscript -->
        </td>
    </tr>

    <tr>
        <td><label for="read_authors"><?php echo PERM_READ; ?></label></td>
        <td>
            <select size="6" id="read_authors" multiple="multiple" name="serendipity[cat][read_authors][]">
                <option value="0" <?php echo (!is_array($this_cat) || (isset($this_cat['authorid']) && $this_cat['authorid'] == '0') || isset($read_groups[0])) ? 'selected="selected"' : ''; ?>><?php echo ALL_AUTHORS; ?></option>
<?php
        foreach($groups AS $group) {
            echo '<option value="' . $group['confkey'] . '" ' . (isset($read_groups[$group['confkey']]) ? 'selected="selected"' : '') . '>' . htmlspecialchars($group['confvalue']) . '</option>' . "\n";
        }
?>
            </select>
        </td>
    </tr>

    <tr>
        <td><label for="write_authors"><?php echo PERM_WRITE; ?></label></td>
        <td>
            <select size="6" id="write_authors" multiple="multiple" name="serendipity[cat][write_authors][]">
                <option value="0" <?php echo (!is_array($this_cat) || (isset($this_cat['authorid']) && $this_cat['authorid'] == '0') || isset($write_groups[0])) ? 'selected="selected"' : ''; ?>><?php echo ALL_AUTHORS; ?></option>
<?php
        foreach($groups AS $group) {
            echo '<option value="' . $group['confkey'] . '" ' . (isset($write_groups[$group['confkey']]) ? 'selected="selected"' : '') . '>' . htmlspecialchars($group['confvalue']) . '</option>' . "\n";
        }
?>
            </select>
        </td>
    </tr>

    <tr>
        <td><label for="parent_cat"><?php echo PARENT_CATEGORY; ?></label></td>
        <td>
            <select id="parent_cat" name="serendipity[cat][parent_cat]">
                <option value="0"<?php if ( (int)$serendipity['GET']['cid'] == 0 ) echo ' selected="selected"'; ?>>[ <?php echo NO_CATEGORY; ?> ]</option>
<?php
        $categories = serendipity_fetchCategories('all');
        $categories = serendipity_walkRecursive($categories, 'categoryid', 'parentid', VIEWMODE_THREADED);
        foreach ( $categories as $cat ) {
            /* We can't be our own parent, the universe will collapse */
            if ( $cat['categoryid'] == $serendipity['GET']['cid'] ) {
                continue;
            }
            echo '<option value="'. $cat['categoryid'] .'"'. ($this_cat['parentid'] == $cat['categoryid'] ? ' selected="selected"' : '') .'>'. str_repeat('&nbsp;', $cat['depth']) . $cat['category_name'] .'</option>' . "\n";
        }
?>
            </select>
        </td>
    </tr>
    <?php serendipity_plugin_api::hook_event('backend_category_showForm', $cid, $this_cat); ?>
</table>
    <div><input type="submit" name="SAVE" value="<?php echo $save; ?>" class="serendipityPrettyButton" /></div>
</form>
<?php } ?>




<?php
if ( $serendipity['GET']['adminAction'] == 'view' ) {
    if (empty($admin_category)) {
        $cats = serendipity_fetchCategories('all');
    } else {
        $cats = serendipity_fetchCategories(null, null, null, 'write');
    }

    if ( is_array($cats) && sizeof($cats) > 0 ) {
        echo CATEGORY_INDEX .':';
    } else {
        echo '<div align="center">- '. NO_CATEGORIES .' -</div>';
    }
?>
<br /><br />
<table cellspacing="0" cellpadding="4" width="100%" border=0>
<?php
            if ( is_array($cats) ) {
                $categories = serendipity_walkRecursive($cats, 'categoryid', 'parentid', VIEWMODE_THREADED);
                foreach ( $categories as $category ) {
?>
            <tr>
                <td width="16"><a href="?serendipity[adminModule]=category&amp;serendipity[adminAction]=edit&amp;serendipity[cid]=<?php echo $category['categoryid'] ?>"><img src="<?php echo serendipity_getTemplateFile('admin/img/edit.png') ?>" border="0" alt="<?php echo EDIT ?>" /></a></td>
                <td width="16"><a href="?serendipity[adminModule]=category&amp;serendipity[adminAction]=delete&amp;serendipity[cid]=<?php echo $category['categoryid'] ?>"><img src="<?php echo serendipity_getTemplateFile('admin/img/delete.png') ?>" border="0" alt="<?php echo DELETE ?>" /></a></td>
                <td width="16"><?php if ( !empty($category['category_icon']) ) {?><img src="<?php echo serendipity_getTemplateFile('admin/img/thumbnail.png') ?>" alt="" /><?php } else echo '&nbsp;' ?></td>
                <td width="300" style="padding-left: <?php echo ($category['depth']*15)+20 ?>px"><img src="<?php echo serendipity_getTemplateFile('admin/img/folder.png') ?>" style="vertical-align: bottom;"> <?php echo htmlspecialchars($category['category_name']) ?></td>
                <td><?php echo htmlspecialchars($category['category_description']) ?></td>
                <td align="right"><?php echo ($category['authorid'] == '0' ? ALL_AUTHORS : $category['realname']); ?></td>
            </tr>
<?php           }
            } ?>
            <tr>
                <td colspan="6" align="right">
                    <a href="?serendipity[adminModule]=category&serendipity[adminAction]=new" class="serendipityPrettyButton"><?php echo CREATE_NEW_CAT ?></a>
                </td>
            </tr>
</table>
<?php }

/* vim: set sts=4 ts=4 expandtab : */
