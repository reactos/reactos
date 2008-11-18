<?php # $Id: export.inc.php 7 2005-04-16 06:39:31Z s_bergmann $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

if (IN_serendipity !== true) {
    die ("Don't hack!");
}

?>
  <div>
    <a href="<?php echo $serendipity['baseURL'] ?>rss.php?version=2.0&all=1" class="serendipityPrettyButton"><?php echo EXPORT_FEED; ?></a>
  </div>
<?php
/* vim: set sts=4 ts=4 expandtab : */
?>
