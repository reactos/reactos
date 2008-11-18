<?php

/** Alemannic (Alemannisch)
 *
 * @ingroup Language
 */
class LanguageGsw extends Language {
   # Convert from the nominative form of a noun to some other case
   # Invoked with result

   function convertGrammar( $word, $case ) {
       global $wgGrammarForms;
       if ( isset($wgGrammarForms['gsw'][$case][$word]) ) {
           return $wgGrammarForms['gsw'][$case][$word];
       }
       switch ( $case ) {
           case 'dativ':
               if ( $word == 'Wikipedia' ) {
                   $word = 'vo de Wikipedia';
               } elseif ( $word == 'Wikinorchrichte' ) {
                   $word = 'vo de Wikinochrichte';
               } elseif ( $word == 'Wiktionaire' ) {
                   $word = 'vom Wiktionaire';
               } elseif ( $word == 'Wikibuecher' ) {
                   $word = 'vo de Wikibuecher';
               } elseif ( $word == 'Wikisprüch' ) {
                   $word = 'vo de Wikisprüch';
               } elseif ( $word == 'Wikiquälle' ) {
                   $word = 'vo de Wikiquälle';
               }
               break;
           case 'akkusativ':
               if ( $word == 'Wikipedia' ) {
                   $word = 'd Wikipedia';
               } elseif ( $word == 'Wikinorchrichte' ) {
                   $word = 'd Wikinochrichte';
               } elseif ( $word == 'Wiktionaire' ) {
                   $word = 's Wiktionaire';
               } elseif ( $word == 'Wikibuecher' ) {
                   $word = 'd Wikibuecher';
               } elseif ( $word == 'Wikisprüch' ) {
                   $word = 'd Wikisprüch';
               } elseif ( $word == 'Wikiquälle' ) {
                   $word = 'd Wikiquälle';
               }
               break;
           case 'nominativ':
               if ( $word == 'Wikipedia' ) {
                   $word = 'd Wikipedia';
               } elseif ( $word == 'Wikinorchrichte' ) {
                   $word = 'd Wikinochrichte';
               } elseif ( $word == 'Wiktionaire' ) {
                   $word = 's Wiktionaire';
               } elseif ( $word == 'Wikibuecher' ) {
                   $word = 'd Wikibuecher';
               } elseif ( $word == 'Wikisprüch' ) {
                   $word = 'd Wikisprüch';
               } elseif ( $word == 'Wikiquälle' ) {
                   $word = 'd Wikiquälle';
               }
               break;
       }
       return $word;
   }
}
