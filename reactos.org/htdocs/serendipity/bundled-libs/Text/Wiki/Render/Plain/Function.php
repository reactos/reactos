<?php

// $Id: Function.php,v 1.3 2004/10/08 17:46:47 pmjones Exp $

class Text_Wiki_Render_Plain_Function extends Text_Wiki_Render {
    
    /**
    * 
    * Renders a token into text matching the requested format.
    * 
    * @access public
    * 
    * @param array $options The "options" portion of the token (second
    * element).
    * 
    * @return string The text rendered from the token options.
    * 
    */
    
    function token($options)
    {
        extract($options); // access, return, name, params, throws
        
        $output = "$access $return $name ( ";
        
        foreach ($params as $key => $val) {
            $output .= "{$val['type']} {$val['descr']} {$val['default']} ";
        }
        
        $output .= ') ';
        
        foreach ($throws as $key => $val) {
            $output .= "{$val['type']} {$val['descr']} ";
        }
        
        return $output;
    }
}
?>