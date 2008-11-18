<?php
//
// +----------------------------------------------------------------------+
// | PHP Version 4                                                        |
// +----------------------------------------------------------------------+
// | Copyright (c) 1997-2002 The PHP Group                                |
// +----------------------------------------------------------------------+
// | This source file is subject to version 2.0 of the PHP license,       |
// | that is bundled with this package in the file LICENSE, and is        |
// | available at through the world-wide-web at                           |
// | http://www.php.net/license/2_02.txt.                                 |
// | If you did not receive a copy of the PHP license and are unable to   |
// | obtain it through the world-wide-web, please send a note to          |
// | license@php.net so we can mail you a copy immediately.               |
// +----------------------------------------------------------------------+
// | Authors: Martin Jansen <mj@php.net>                                  |
// |          Guido Haeger <gh-lists@ecora.de>                            |
// +----------------------------------------------------------------------+
//
// $Id: CheckIP.php,v 1.5 2002/08/17 09:41:24 mj Exp $

/**
* Class to validate the syntax of IPv4 adresses
*
* Usage:
*   <?php
*   require_once "Net/CheckIP.php";
*     
*   if (Net_CheckIP::check_ip("your_ip_goes_here")) {
*       // Syntax of the IP is ok
*   }
*   ?>
*
* @author  Martin Jansen <mj@php.net>
* @author  Guido Haeger <gh-lists@ecora.de>
* @package Net_CheckIP
* @version 1.1
* @access  public
*/
class Net_CheckIP
{

    /**
    * Validate the syntax of the given IP adress
    *
    * This function splits the IP address in 4 pieces
    * (separated by ".") and checks for each piece
    * if it's an integer value between 0 and 255.
    * If all 4 parameters pass this test, the function
    * returns true.
    *
    * @param  string $ip IP adress
    * @return bool       true if syntax is valid, otherwise false
    */
    function check_ip($ip)
    {
        $oct = explode('.', $ip);
        if (count($oct) != 4) {
            return false;
        }

        for ($i = 0; $i < 4; $i++) {
            if (!is_numeric($oct[$i])) {
                return false;
            }

            if ($oct[$i] < 0 || $oct[$i] > 255) {
                return false;
            }
        }

        return true;
    }
}
?>
