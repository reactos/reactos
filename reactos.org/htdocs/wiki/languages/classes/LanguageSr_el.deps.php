<?php
// This file exists to ensure that base classes are preloaded before
// LanguageSr_el.php is compiled, working around a bug in the APC opcode
// cache on PHP 5, where cached code can break if the include order
// changed on a subsequent page view.
// see http://mail.wikipedia.org/pipermail/wikitech-l/2006-January/033660.html

require_once( dirname(__FILE__).'/LanguageSr_ec.php' );
