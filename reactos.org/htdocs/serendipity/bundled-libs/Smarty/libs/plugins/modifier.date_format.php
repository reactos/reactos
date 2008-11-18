<?php
/**
 * Smarty plugin
 * @package Smarty
 * @subpackage plugins
 */

/**
 * Include the {@link shared.make_timestamp.php} plugin
 */
require_once $smarty->_get_plugin_filepath('shared','make_timestamp');
/**
 * Smarty date_format modifier plugin
 *
 * Type:     modifier<br>
 * Name:     date_format<br>
 * Purpose:  format datestamps via strftime<br>
 * Input:<br>
 *         - string: input date string
 *         - format: strftime format for output
 *         - default_date: default date if $string is empty
 * @link http://smarty.php.net/manual/en/language.modifier.date.format.php
 *          date_format (Smarty online manual)
 * @param string
 * @param string
 * @param string
 * @return string|void
 * @uses smarty_make_timestamp()
 */
function smarty_modifier_date_format($string, $format="%b %e, %Y", $default_date=null)
{
    if (substr(PHP_OS,0,3) == 'WIN') {
           $_win_from = array ('%e',  '%T',       '%D');
           $_win_to   = array ('%#d', '%H:%M:%S', '%m/%d/%y');
           $format = str_replace($_win_from, $_win_to, $format);
    }
    if($string != '') {
        return strftime($format, smarty_make_timestamp($string));
    } elseif (isset($default_date) && $default_date != '') {
        return strftime($format, smarty_make_timestamp($default_date));
    } else {
        return;
    }
}

/* vim: set expandtab: */

?>
