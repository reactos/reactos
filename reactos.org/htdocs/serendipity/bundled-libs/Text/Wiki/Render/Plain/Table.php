<?php

class Text_Wiki_Render_Plain_Table extends Text_Wiki_Render {
    
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
        // make nice variable names (type, attr, span)
        extract($options);
        
        $pad = '    ';
        
        switch ($type) {
        
        case 'table_start':
            return;
            break;
        
        case 'table_end':
            return;
            break;
        
        case 'row_start':
            return;
            break;
        
        case 'row_end':
            return " ||\n";
            break;
        
        case 'cell_start':
            return " || ";
            break;
        
        case 'cell_end':
            return;
            break;
        
        default:
            return '';
        
        }
    }
}
?>