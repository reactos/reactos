<?php # $Id:$

##########################################################################
# serendipity - another blogger...                                       #
##########################################################################
#                                                                        #
# (c) 2003 Jannis Hermanns <J@hacked.it>                                 #
# http://www.jannis.to/programming/serendipity.html                      #
#                                                                        #
# Translated by                                                          #
# João P Matos <jmatos@math.ist.utl.pt>                                  #
#                                                                        #
##########################################################################

@define('PLUGIN_EVENT_CONTENTREWRITE_FROM', 'de');
@define('PLUGIN_EVENT_CONTENTREWRITE_TO', 'para');
@define('PLUGIN_EVENT_CONTENTREWRITE_NAME', 'Reescrita de conteúdo');
@define('PLUGIN_EVENT_CONTENTREWRITE_DESCRIPTION', 'Substitui palavras por um texto pré definido (prático para as abreviaturas)');
@define('PLUGIN_EVENT_CONTENTREWRITE_NEWTITLE', 'Novo título');
@define('PLUGIN_EVENT_CONTENTREWRITE_NEWTDESCRIPTION', 'Introduza o título da abreviatura para a nova entrada ({de})');
@define('PLUGIN_EVENT_CONTENTREWRITE_OLDTITLE', 'Título #%d');
@define('PLUGIN_EVENT_CONTENTREWRITE_OLDTDESCRIPTION', 'Introduza a abreviatura aqui ({de})');
@define('PLUGIN_EVENT_CONTENTREWRITE_PTITLE', 'Título do plugin');
@define('PLUGIN_EVENT_CONTENTREWRITE_PDESCRIPTION', 'O nome deste plugin');
@define('PLUGIN_EVENT_CONTENTREWRITE_NEWDESCRIPTION', 'Nova descrição');
@define('PLUGIN_EVENT_CONTENTREWRITE_NEWDDESCRIPTION', 'Introduza a descrição para a nova entrada ({para})');
@define('PLUGIN_EVENT_CONTENTREWRITE_OLDDESCRIPTION', 'Descrição #%s');
@define('PLUGIN_EVENT_CONTENTREWRITE_OLDDDESCRIPTION', 'Introduza a descrição aqui ({para})');
@define('PLUGIN_EVENT_CONTENTREWRITE_REWRITESTRING', 'Texto de substituição');
@define('PLUGIN_EVENT_CONTENTREWRITE_REWRITESTRINGDESC', 'Introduza o texto com o qual pretende substituir a palavra que escolheu. Vous pouvez utiliserPode utilizar {de} e {para} onde desejar para juntar uma reescrita.' . "\n" . 'Exemplo: <acronym title="{vers}">{de}</acronym>');
@define('PLUGIN_EVENT_CONTENTREWRITE_REWRITECHAR', 'Caracter de reescrita');
@define('PLUGIN_EVENT_CONTENTREWRITE_REWRITECHARDESC', 'Se utiliza um caracter especial para forçar a reescrita, introduza-lo aqui. Exemplo: se deseja somente substituir \'palavra*\' com o texto que definiu, mas não quer que o \'*\' seja mostrados, introduza o caracter aqui.');

/* vim: set sts=4 ts=4 expandtab : */
?>
