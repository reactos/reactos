<?php # $Id: entries.inc.php 540 2005-10-06 09:36:53Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

if (IN_serendipity !== true) {
    die ("Don't hack!");
}

if (!serendipity_checkPermission('adminEntries')) {
    return;
}

$sort_order = array('timestamp'     => DATE,
                    'isdraft'       => PUBLISH . '/' . DRAFT,
                    'a.realname'    => AUTHOR,
                    'category_name' => CATEGORY,
                    'last_modified' => LAST_UPDATED,
                    'title'         => TITLE);
$per_page = array('12', '16', '50', '100');


// A little helper we don't want in _functions.inc.php
function serendipity_drawList() {
    global $serendipity, $sort_order, $per_page;

    $filter_import = array('author', 'category', 'isdraft');
    $sort_import   = array('perPage', 'ordermode', 'order');
    foreach($filter_import AS $f_import) {
        serendipity_restoreVar($serendipity['COOKIE']['entrylist_filter_' . $f_import], $serendipity['GET']['filter'][$f_import]);
        serendipity_JSsetCookie('entrylist_filter_' . $f_import, $serendipity['GET']['filter'][$f_import]);
    }

    foreach($sort_import AS $s_import) {
        serendipity_restoreVar($serendipity['COOKIE']['entrylist_sort_' . $s_import], $serendipity['GET']['sort'][$s_import]);
        serendipity_JSsetCookie('entrylist_sort_' . $s_import, $serendipity['GET']['sort'][$s_import]);
    }

    $perPage = (!empty($serendipity['GET']['sort']['perPage']) ? $serendipity['GET']['sort']['perPage'] : $per_page[0]);
    $page    = (int)$serendipity['GET']['page'];
    $offSet  = $perPage*$page;

    if (empty($serendipity['GET']['sort']['ordermode']) || $serendipity['GET']['sort']['ordermode'] != 'ASC') {
        $serendipity['GET']['sort']['ordermode'] = 'DESC';
    }

    if (!empty($serendipity['GET']['sort']['order']) && !empty($sort_order[$serendipity['GET']['sort']['order']])) {
        $orderby = serendipity_db_escape_string($serendipity['GET']['sort']['order'] . ' ' . $serendipity['GET']['sort']['ordermode']);
    } else {
        $orderby = 'timestamp ' . serendipity_db_escape_string($serendipity['GET']['sort']['ordermode']);
    }

    $filter = array();

    if (!empty($serendipity['GET']['filter']['author'])) {
        $filter[] = "e.authorid = '" . serendipity_db_escape_string($serendipity['GET']['filter']['author']) . "'";
    }

    if (!empty($serendipity['GET']['filter']['category'])) {
        $filter[] = "ec.categoryid = '" . serendipity_db_escape_string($serendipity['GET']['filter']['category']) . "'";
    }

    if (!empty($serendipity['GET']['filter']['isdraft'])) {
        if ($serendipity['GET']['filter']['isdraft'] == 'draft') {
            $filter[] = "e.isdraft = 'true'";
        } elseif ($serendipity['GET']['filter']['isdraft'] == 'publish') {
            $filter[] = "e.isdraft = 'false'";
        }
    }

    if (!empty($serendipity['GET']['filter']['body'])) {
        if ($serendipity['dbType'] == 'mysql') {
            $filter[] = "MATCH (title,body,extended) AGAINST ('" . serendipity_db_escape_string($serendipity['GET']['filter']['body']) . "')";
            $full     = true;
        }
    }

    $filter_sql = implode(' AND ', $filter);

    // Fetch the entries
    $entries = serendipity_fetchEntries(
                 false,
                 false,
                 serendipity_db_limit(
                   $offSet,
                   $perPage
                 ),
                 true,
                 false,
                 $orderby,
                 $filter_sql
               );
?>
<form action="?" method="get">
<div class="serendipity_admin_list">
    <input type="hidden" name="serendipity[action]"      value="admin"      />
    <input type="hidden" name="serendipity[adminModule]" value="entries"    />
    <input type="hidden" name="serendipity[adminAction]" value="editSelect" />
    <table width="100%" class="serendipity_admin_filters">
        <tr>
            <td class="serendipity_admin_filters_headline" colspan="6"><strong><?php echo FILTERS ?></strong> - <?php echo FIND_ENTRIES ?></td>
        </tr>
        <tr>
            <td valign="top" width="80"><?php echo AUTHOR ?></td>
            <td valign="top">
                <select name="serendipity[filter][author]">
                    <option value="">--</option>
<?php
                    $users = serendipity_fetchUsers();
                    if (is_array($users)) {
                        foreach ($users AS $user) {
                            echo '<option value="' . $user['authorid'] . '" ' . (isset($serendipity['GET']['filter']['author']) && $serendipity['GET']['filter']['author'] == $user['authorid'] ? 'selected="selected"' : '') . '>' . $user['realname'] . '</option>' . "\n";
                        }
                    }
?>              </select> <select name="serendipity[filter][isdraft]">
                    <option value="all"><?php echo COMMENTS_FILTER_ALL; ?></option>
                    <option value="draft"   <?php echo (isset($serendipity['GET']['filter']['isdraft']) &&  $serendipity['GET']['filter']['isdraft'] == 'draft' ? 'selected="selected"' : ''); ?>><?php echo DRAFT; ?></option>
                    <option value="publish" <?php echo (isset($serendipity['GET']['filter']['isdraft']) &&  $serendipity['GET']['filter']['isdraft'] == 'publish' ? 'selected="selected"' : ''); ?>><?php echo PUBLISH; ?></option>
                </select>
            </td>
            <td valign="top" width="80"><?php echo CATEGORY ?></td>
            <td valign="top">
                <select name="serendipity[filter][category]">
                    <option value="">--</option>
<?php
                    $categories = serendipity_fetchCategories();
                    $categories = serendipity_walkRecursive($categories, 'categoryid', 'parentid', VIEWMODE_THREADED);
                    foreach ( $categories as $cat ) {
                        echo '<option value="'. $cat['categoryid'] .'"'. ($serendipity['GET']['filter']['category'] == $cat['categoryid'] ? ' selected="selected"' : '') .'>'. str_repeat('&nbsp;', $cat['depth']) . $cat['category_name'] .'</option>' . "\n";
                    }
?>              </select>
            </td>
            <td valign="top" width="80"><?php echo CONTENT ?></td>
            <td valign="top"><input size="10" type="text" name="serendipity[filter][body]" value="<?php echo (isset($serendipity['GET']['filter']['body']) ? htmlspecialchars($serendipity['GET']['filter']['body']) : '') ?>" /></td>
        </tr>
        <tr>
            <td class="serendipity_admin_filters_headline" colspan="6"><strong><?php echo SORT_ORDER ?></strong></td>
        </tr>
        <tr>
            <td>
                <?php echo SORT_BY ?>
            </td>
            <td>
                <select name="serendipity[sort][order]">
<?php
    foreach($sort_order as $so_key => $so_val) {
        echo '<option value="' . $so_key . '" ' . (isset($serendipity['GET']['sort']['order']) && $serendipity['GET']['sort']['order'] == $so_key ? 'selected="selected"': '') . '>' . $so_val . '</option>' . "\n";
    }
?>              </select>
            </td>
            <td><?php echo SORT_ORDER ?></td>
            <td>
                <select name="serendipity[sort][ordermode]">
                    <option value="DESC" <?php echo (isset($serendipity['GET']['sort']['ordermode']) && $serendipity['GET']['sort']['ordermode'] == 'DESC' ? 'selected="selected"' : '') ?>><?php echo SORT_ORDER_DESC ?></option>
                    <option value="ASC" <?php echo (isset($serendipity['GET']['sort']['ordermode']) && $serendipity['GET']['sort']['ordermode'] == 'ASC'  ? 'selected="selected"' : '') ?>><?php echo SORT_ORDER_ASC ?></option>
                </select>
            </td>
            <td><?php echo ENTRIES_PER_PAGE ?></td>
            <td>
                <select name="serendipity[sort][perPage]">
<?php
    foreach($per_page AS $per_page_nr) {
       echo '<option value="' . $per_page_nr . '"   ' . (isset($serendipity['GET']['sort']['perPage']) && $serendipity['GET']['sort']['perPage'] == $per_page_nr ? 'selected="selected"' : '') . '>' . $per_page_nr . '</option>' . "\n";
    }

?>
                </select>
            </td>
        </tr>
        <tr>
            <td align="right" colspan="6"><input type="submit" name="go" value="<?php echo GO ?>" class="serendipityPrettyButton" /></td>
        </tr>
    </table>
    <table class="serendipity_admin_list" cellpadding="5" width="100%">
<?php
    if (is_array($entries)) {
        $count = count($entries);
        $qString = '?serendipity[adminModule]=entries&amp;serendipity[adminAction]=editSelect';
        foreach ((array)$serendipity['GET']['sort'] as $k => $v) {
            $qString .= '&amp;serendipity[sort]['. $k .']='. $v;
        }
        foreach ((array)$serendipity['GET']['filter'] as $k => $v) {
            $qString .= '&amp;serendipity[filter]['. $k .']='. $v;
        }
        $linkPrevious = $qString . '&amp;serendipity[page]=' . ($page-1);
        $linkNext     = $qString . '&amp;serendipity[page]=' . ($page+1);
?>
        <tr>
            <td>
                <?php if ($offSet > 0) { ?>
                    <a href="<?php echo $linkPrevious ?>" class="serendipityIconLink"><img src="<?php echo serendipity_getTemplateFile('admin/img/previous.png') ?>" /><?php echo PREVIOUS ?></a>
                <?php } ?>
            </td>
            <td align="right">
                <?php if ($count == $perPage) { ?>
                    <a href="<?php echo $linkNext ?>" class="serendipityIconLinkRight"><?php echo NEXT ?><img src="<?php echo serendipity_getTemplateFile('admin/img/next.png') ?>" /></a>
                <?php } ?>
            </td>
        </tr>
    </table>
<?php
        // Print the entries
        $rows = 0;
        foreach ( $entries as $entry ) {
            $rows++;
            // Find out if the entry has been modified later than 30 minutes after creation
            if ($entry['timestamp'] <= ($entry['last_modified'] - 60*30)) {
                $lm = '<a href="#" title="' . LAST_UPDATED . ': ' . serendipity_formatTime(DATE_FORMAT_SHORT, $entry['last_modified']) . '" onclick="alert(this.title)"><img src="'. serendipity_getTemplateFile('admin/img/clock.png') .'" alt="*" style="border: 0px none ; vertical-align: bottom;" /></a>';
            } else {
                $lm = '';
            }

            if (!$serendipity['showFutureEntries'] && $entry['timestamp'] >= serendipity_serverOffsetHour()) {
                $entry_pre = '<a href="#" title="' . ENTRY_PUBLISHED_FUTURE . '" onclick="alert(this.title)"><img src="'. serendipity_getTemplateFile('admin/img/clock_future.png') .'" alt="*" style="border: 0px none ; vertical-align: bottom;" /></a> ';
            } else {
                $entry_pre = '';
            }

            if (serendipity_db_bool($entry['isdraft'])) {
                $entry_pre .= ' ' . DRAFT . ': ';
            }
?>
            <div class="serendipity_admin_list_item serendipity_admin_list_item_<?php echo ($rows % 2 ? 'even' : 'uneven'); ?>">
                <table width="100%" cellspacing="0" cellpadding="3">
                    <tr>
                        <td>
                            <strong><?php echo $entry_pre; ?><a href="?serendipity[action]=admin&amp;serendipity[adminModule]=entries&amp;serendipity[adminAction]=edit&amp;serendipity[id]=<?php echo $entry['id']; ?>" title="#<?php echo $entry['id']; ?>"><?php echo serendipity_truncateString($entry['title'],50) ?></a></strong>
                        </td>
                        <td align="right">
                            <?php echo serendipity_formatTime(DATE_FORMAT_SHORT, $entry['timestamp']) . ' ' .$lm; ?>
                        </td>
                    </tr>
                    <tr>
                        <td>
                            <?php
                echo POSTED_BY . ' ' . $entry['author'];
                if (count($entry['categories'])) {
                    echo ' ' . IN . ' ';
                    $cats = array();
                    foreach ($entry['categories'] as $cat) {
                        $cats[] = '<a href="' . $serendipity['serendipityHTTPPath'] . ($serendipity['rewrite'] == 'none' ? $serendipity['indexFile'] . '?/' : '') . 'categories/' . $cat['categoryid'] . '_' . serendipity_makeFilename($cat['category_name']) . '">' . $cat['category_name'] . '</a>';
                    }
                    echo implode(', ', $cats);
                }
                ?>

                        </td>
                        <td align="right">
                            <a href="?serendipity[action]=admin&amp;serendipity[adminModule]=entries&amp;serendipity[adminAction]=edit&amp;serendipity[id]=<?php echo $entry['id']; ?>" title="<?php echo EDIT . ' #' . $entry['id']; ?>" class="serendipityIconLink"><img src="<?php echo serendipity_getTemplateFile('admin/img/edit.png'); ?>" alt="<?php echo EDIT; ?>" /><?php echo EDIT ?></a>
                            <a href="?serendipity[action]=admin&amp;serendipity[adminModule]=entries&amp;serendipity[adminAction]=delete&amp;serendipity[id]=<?php echo $entry['id']; ?>" title="<?php echo DELETE . ' #' . $entry['id']; ?>" class="serendipityIconLink"><img src="<?php echo serendipity_getTemplateFile('admin/img/delete.png'); ?>" alt="<?php echo DELETE; ?>" /><?php echo DELETE ?></a>
                        </td>
                    </tr>
                </table>
        </div>
<?php
        } // end entries output
?>

        <table class="serendipity_admin_list" cellpadding="5" width="100%">
            <tr>
                <td>
                    <?php if ($offSet > 0) { ?>
                        <a href="<?php echo $linkPrevious ?>" class="serendipityIconLink"><img src="<?php echo serendipity_getTemplateFile('admin/img/previous.png') ?>" /><?php echo PREVIOUS ?></a>
                    <?php } ?>
                </td>
                <td align="right">
                    <?php if ($count == $perPage) { ?>
                        <a href="<?php echo $linkNext ?>" class="serendipityIconLinkRight"><?php echo NEXT ?><img src="<?php echo serendipity_getTemplateFile('admin/img/next.png') ?>" /></a>
                    <?php } ?>
                </td>
            </tr>
        </table>

        <div class="serendipity_admin_list_item serendipity_admin_list_item_<?php echo (($rows+1) % 2 ? 'even' : 'uneven'); ?>">
            <table width="100%" cellspacing="0" cellpadding="3">
                    <tr>
                        <td>
                            <?php echo EDIT_ENTRY ?>: #<input type="text" size="3" name="serendipity[id]" /> <input type="submit" name="serendipity[editSubmit]" value="<?php echo GO ?>" class="serendipityPrettyButton" />
                        </td>
                    </tr>
            </table>
        </div>
 <?php
    } else {
        // We've got nothing
?>
        <div align="center">- <?php echo NO_ENTRIES_TO_PRINT ?> -</div>
<?php
    }
?>
</div>
</form>
<?php
} // End function serendipity_drawList()

if (!empty($serendipity['GET']['editSubmit'])) {
    $serendipity['GET']['adminAction'] = 'edit';
}

switch($serendipity['GET']['adminAction']) {
    case 'save':
        $entry = array(
                   'id'                 => $serendipity['POST']['id'],
                   'title'              => $serendipity['POST']['title'],
                   'timestamp'          => $serendipity['POST']['timestamp'],
                   'body'               => $serendipity['POST']['body'],
                   'extended'           => $serendipity['POST']['extended'],
                   'categories'         => $serendipity['POST']['categories'],
                   'isdraft'            => $serendipity['POST']['isdraft'],
                   'allow_comments'     => $serendipity['POST']['allow_comments'],
                   'moderate_comments'  => $serendipity['POST']['moderate_comments'],
                   'exflag'             => (!empty($serendipity['POST']['extended']) ? true : false)
        );

        if ($entry['allow_comments'] != 'true' && $entry['allow_comments'] !== true) {
            $entry['allow_comments'] = 'false';
        }

        if ($entry['moderate_comments'] != 'true' && $entry['moderate_comments'] !== true) {
            $entry['moderate_comments'] = 'false';
        }

        // Check if the user changed the timestamp.
        if (isset($serendipity['allowDateManipulation']) && $serendipity['allowDateManipulation'] && isset($serendipity['POST']['new_timestamp']) && $serendipity['POST']['new_timestamp'] != date(DATE_FORMAT_2, $serendipity['POST']['chk_timestamp'])) {
            // The user changed the timestamp, now set the DB-timestamp to the user's date
            $entry['timestamp'] = strtotime($serendipity['POST']['new_timestamp']);

            if ($entry['timestamp'] == -1) {
                echo DATE_INVALID . '<br />';
                // The date given by the user is not convertable. Reset the timestamp.
                $entry['timestamp'] = $serendipity['POST']['timestamp'];
            }
        }

        // Save server timezone in database always, so substract the offset we added for display; otherwise it would be added time and again
        if (!empty($entry['timestamp'])) {
            $entry['timestamp'] = serendipity_serverOffsetHour($entry['timestamp'], true);
        }

        // Save the entry, or just display a preview
        if ($serendipity['POST']['preview'] != 'true') {
            /* We don't need an iframe to save a draft */
            if ( $serendipity['POST']['isdraft'] == 'true' ) {
                echo '<div class="serendipityAdminMsgSuccess">' . IFRAME_SAVE_DRAFT . '</div><br />';
                serendipity_updertEntry($entry);
            } else {
                if ($serendipity['use_iframe']) {
                    echo '<div class="serendipityAdminMsgSuccess">' . IFRAME_SAVE . '</div><br />';
                    serendipity_iframe_create('save', $entry);
                } else {
                    serendipity_iframe($entry, 'save');
                }
            }
        } else {
            // Only display the preview
            $serendipity['hidefooter'] = true;
            if (!is_numeric($entry['timestamp'])) {
                $entry['timestamp']  = time();
            }

            if (!isset($entry['trackbacks']) || !$entry['trackbacks']) {
                $entry['trackbacks'] = 0;
            }

            if (!isset($entry['comments']) || !$entry['comments']) {
                $entry['comments']   = 0;
            }

            if (!isset($entry['realname']) || !$entry['realname']) {
                if (!empty($serendipity['realname'])) {
                    $entry['realname']   = $serendipity['realname'];
                } else {
                    $entry['realname']   = $serendipity['serendipityUser'];
                }
            }

            $categories = (array)$entry['categories'];
            $entry['categories'] = array();
            foreach ($categories as $catid) {
                if ($catid == 0) {
                    continue;
                }
                $entry['categories'][] = serendipity_fetchCategoryInfo($catid);
            }

            if (count($entry['categories']) < 1) {
                unset($entry['categories']);
            }

            if (isset($entry['id'])) {
                $serendipity['GET']['id'] = $entry['id'];
            } else {
                $serendipity['GET']['id'] = 1;
            }

            if ($serendipity['use_iframe']) {
                echo '<div class="serendipityAdminMsgSuccess">' . IFRAME_PREVIEW . '</div><br />';
                serendipity_iframe_create('preview', $entry);
            } else {
                serendipity_iframe($entry, 'preview');
            }
        }

        // serendipity_updertEntry sets this global variable to store the entry id. Couldn't pass this
        // by reference or as return value because it affects too many places inside our API and dependant
        // function calls.
        if (!empty($serendipity['lastSavedEntry'])) {
            $entry['id'] = $serendipity['lastSavedEntry'];
        }

        include_once S9Y_INCLUDE_PATH . 'include/functions_entries_admin.inc.php';
        serendipity_printEntryForm(
            '?',
            array(
              'serendipity[action]'      => 'admin',
              'serendipity[adminModule]' => 'entries',
              'serendipity[adminAction]' => 'save',
              'serendipity[timestamp]'   => $entry['timestamp']
            ),

            $entry
        );

        break;

    case 'doDelete':
        serendipity_deleteEntry($serendipity['GET']['id']);
        printf(RIP_ENTRY, $serendipity['GET']['id']);
        echo '<br />';

    case 'editSelect':
        serendipity_drawList();
        break;

    case 'delete':
        $newLoc = '?serendipity[action]=admin&amp;serendipity[adminModule]=entries&amp;serendipity[adminAction]=doDelete&amp;serendipity[id]=' . $serendipity['GET']['id'];
        printf(DELETE_SURE, $serendipity['GET']['id']);
?>
<br />
<br />
<div>
    <a href="<?php echo $_SERVER["HTTP_REFERER"]; ?>" class="serendipityPrettyButton"><?php echo NOT_REALLY; ?></a>
    <?php echo str_repeat('&nbsp;', 10); ?>
    <a href="<?php echo $newLoc; ?>" class="serendipityPrettyButton"><?php echo DUMP_IT; ?></a>
</div>
<?php
        break;

    case 'edit':
        $entry = serendipity_fetchEntry('id', $serendipity['GET']['id'], 1, 1);

    default:
        include_once S9Y_INCLUDE_PATH . 'include/functions_entries_admin.inc.php';

        serendipity_printEntryForm(
            '?',
            array(
            'serendipity[action]'      => 'admin',
            'serendipity[adminModule]' => 'entries',
            'serendipity[adminAction]' => 'save'
            ),
            (isset($entry) ? $entry : array())
        );
}
/* vim: set sts=4 ts=4 expandtab : */
?>
