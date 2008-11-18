<?php # $Id: overview.inc.php 7 2005-04-16 06:39:31Z s_bergmann $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

if (IN_serendipity !== true) {
    die ("Don't hack!");
}

$user = serendipity_fetchAuthor($serendipity['authorid']);
?>

<?php echo WELCOME_BACK ?> <?php echo $user[0]['realname'] ?>.

<?php serendipity_plugin_api::hook_event('backend_frontpage_display', $serendipity); ?>

<?php
/* vim: set sts=4 ts=4 expandtab : */
?>
