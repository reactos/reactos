<?php

class Text_Wiki_Render_Xhtml_Table extends Text_Wiki_Render {
    
    var $conf = array(
        'css_table' => null,
        'css_tr' => null,
        'css_th' => null,
        'css_td' => null
    );
    
    
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
            $css = $this->formatConf(' class="%s"', 'css_table');
            return "\n\n<table$css>\n";
            break;
        
        case 'table_end':
            return "</table>\n\n";
            break;
        
        case 'row_start':
            $css = $this->formatConf(' class="%s"', 'css_tr');
            return "$pad<tr$css>\n";
            break;
        
        case 'row_end':
            return "$pad</tr>\n";
            break;
        
        case 'cell_start':
            
            // base html
            $html = $pad . $pad;
            
            // is this a TH or TD cell?
            if ($attr == 'header') {
                // start a header cell
                $css = $this->formatConf(' class="%s"', 'css_th');
                $html .= "<th$css";
            } else {
                // start a normal cell
                $css = $this->formatConf(' class="%s"', 'css_td');
                $html .= "<td$css";
            }
            
            // add the column span
            if ($span > 1) {
                $html .= " colspan=\"$span\"";
            }
            
            // add alignment
            if ($attr != 'header' && $attr != '') {
                $html .= " style=\"text-align: $attr;\"";
            }
            
            // done!
            $html .= '>';
            return $html;
            break;
        
        case 'cell_end':
            if ($attr == 'header') {
                return "</th>\n";
            } else {
                return "</td>\n";
            }
            break;
        
        default:
            return '';
        
        }
    }
}
?>