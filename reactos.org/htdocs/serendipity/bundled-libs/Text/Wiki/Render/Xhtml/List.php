<?php


class Text_Wiki_Render_Xhtml_List extends Text_Wiki_Render {
    
    var $conf = array(
        'css_ol' => null,
        'css_ol_li' => null,
        'css_ul' => null,
        'css_ul_li' => null
    );
    
    /**
    * 
    * Renders a token into text matching the requested format.
    * 
    * This rendering method is syntactically and semantically compliant
    * with XHTML 1.1 in that sub-lists are part of the previous list item.
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
        // make nice variables (type, level, count)
        extract($options);
        
        // set up indenting so that the results look nice; we do this
        // in two steps to avoid str_pad mathematics.  ;-)
        $pad = str_pad('', $level, "\t");
        $pad = str_replace("\t", '    ', $pad);
                
        switch ($type) {
        
        case 'bullet_list_start':
        
            // build the base HTML
            $css = $this->formatConf(' class="%s"', 'css_ul');
            $html = "<ul$css>";
            
            // if this is the opening block for the list,
            // put an extra newline in front of it so the
            // output looks nice.
            if ($level == 0) {
                $html = "\n$html";
            }
            
            // done!
            return $html;
            break;
        
        case 'bullet_list_end':
        
            // build the base HTML
            $html = "</li>\n$pad</ul>";
            
            // if this is the closing block for the list,
            // put extra newlines after it so the output
            // looks nice.
            if ($level == 0) {
                $html .= "\n\n";
            }
            
            // done!
            return $html;
            break;
        
        case 'number_list_start':
        
            // build the base HTML
            $css = $this->formatConf(' class="%s"', 'css_ol');
            $html = "<ol$css>";
            
            // if this is the opening block for the list,
            // put an extra newline in front of it so the
            // output looks nice.
            if ($level == 0) {
                $html = "\n$html";
            }
            
            // done!
            return $html;
            break;
        
        case 'number_list_end':
        
            // build the base HTML
            $html = "</li>\n$pad</ol>";
            
            // if this is the closing block for the list,
            // put extra newlines after it so the output
            // looks nice.
            if ($level == 0) {
                $html .= "\n\n";
            }
            
            // done!
            return $html;
            break;
        
        case 'bullet_item_start':
        case 'number_item_start':
        
            // pick the proper CSS class
            if ($type == 'bullet_item_start') {
                $css = $this->formatConf(' class="%s"', 'css_ul_li');
            } else {
                $css = $this->formatConf(' class="%s"', 'css_ol_li');
            }
            
            // build the base HTML
            $html = "\n$pad<li$css>";
            
            // for the very first item in the list, do nothing.
            // but for additional items, be sure to close the
            // previous item.
            if ($count > 0) {
                $html = "</li>$html";
            }
            
            // done!
            return $html;
            break;
        
        case 'bullet_item_end':
        case 'number_item_end':
        default:
            // ignore item endings and all other types.
            // item endings are taken care of by the other types
            // depending on their place in the list.
            return '';
            break;
        }
    }
}
?>