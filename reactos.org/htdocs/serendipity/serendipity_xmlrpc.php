<?php # $Id: serendipity_xmlrpc.php 386 2005-08-11 09:28:54Z garvinhicking $
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details

require_once 'serendipity_config.inc.php';

$data = array();
serendipity_plugin_api::hook_event('frontend_xmlrpc', $data);

if (count($data) == 0) {
    die(XMLRPC_NO_LONGER_BUNDLED);
}
