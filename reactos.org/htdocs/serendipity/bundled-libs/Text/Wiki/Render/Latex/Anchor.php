<?php

/**
* 
* This class renders an anchor target name in LaTeX.
*
* $Id: Anchor.php,v 1.2 2004/09/25 19:05:13 pmjones Exp $
* 
* @author Jeremy Cowgar <jeremy@cowgar.com>
*
* @package Text_Wiki
*
*/

class Text_Wiki_Render_Latex_Anchor extends Text_Wiki_Render {
    
    function token($options)
    {
        extract($options); // $type, $name
        
        if ($type == 'start') {
            //return sprintf('<a id="%s">',$name);
            return '';
        }
        
        if ($type == 'end') {
            //return '</a>';
            return '';
        }
    }
}

?>
