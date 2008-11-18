<?php # $Id: serendipity_define.js.php 7 2005-04-16 06:39:31Z s_bergmann $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

header('Content-type: application/x-javascript');
include_once('serendipity_config.inc.php');
?>
<!-- // Hide from older browsers
// This page serves to carry through any variables without having to parse a complete .js file as .php

var XHTML11 = <?php echo ($serendipity['XHTML11'] ? 'true' : 'false'); ?>;

// -->
<?php
/* vim: set sts=4 ts=4 expandtab : */
?>
