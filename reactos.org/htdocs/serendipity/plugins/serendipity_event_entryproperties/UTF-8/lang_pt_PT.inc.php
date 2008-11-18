<?php # $Id:$
# Copyright (c) 2003-2005, Jannis Hermanns (on behalf the Serendipity Developer Team)
# All rights reserved.  See LICENSE file for licensing details
# Translation (c) by  Joao P Matos <jmatos@math.ist.utl.pt>
/* vim: set sts=4 ts=4 expandtab : */

  //Sticky post
@define('PLUGIN_EVENT_ENTRYPROPERTIES_TITLE', 'Propriedades extra das entradas');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_DESC', '(cache, artigos privados, sticky posts)'); 
@define('PLUGIN_EVENT_ENTRYPROPERTIES_STICKYPOSTS', 'Marcar esta entrada como Sticky Post');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS', 'As entradas podem ser lidas por');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS_PRIVATE', 'Eu próprio');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS_MEMBERS', 'Co-autores');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_ACCESS_PUBLIC', 'Toda a gente');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE', 'Fazer cache das entradas?');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_DESC', 'Se activado, uma versão em cache será gerada sempre que guardar. A cache melhorará o desempenho, mas pode diminuir a flexibilidade para outros plugins.');
@define('PLUGIN_EVENT_ENTRYPROPERTY_BUILDCACHE', 'Construir entradas em cache');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_FETCHNEXT', 'Construindo o conjunto de entradas seguinte...');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_FETCHNO', 'Construindo as entradas %d a %d');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_BUILDING', 'Construindo a cache para a entrada #%d, <em>%s</em>...');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHED', 'Cache construída.');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_DONE', 'Processo completo.');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_ABORTED', 'Processo ABORTADO.');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CACHE_TOTAL', ' (totalizando %d entradas)...');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_NL2BR', 'Inactivar nl2br');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_NO_FRONTPAGE', 'Esconder do resumo de artigo / primeira página');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_GROUPS', 'Utilizar restrições em função de grupos');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_GROUPS_DESC', 'Se activado, pode definir que utilizadores de um grupo estão autorizados a ler as entradas. Esta opção tem impacto considerável no desempenho ao visualizarem-se os seus artigos. Active somente se for de facto usar esta característica.');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_USERS', 'Utilizar restrições em função dos utilizadores');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_USERS_DESC', 'Se activado, pode definir que utilizadores específicos estão autorizados a ler as suas entradas. Esta opção tem impacto considerável no desempenho ao visualizarem-se os seus artigos. Active somente se for de facto usar esta característica.');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_HIDERSS', 'Esconder conteúdo em RSS');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_HIDERSS_DESC', 'Se activado, o conteúdo desta entrada não será visível na sindicalização RSS.');

@define('PLUGIN_EVENT_ENTRYPROPERTIES_CUSTOMFIELDS', 'Campos ad hoc');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CUSTOMFIELDS_DESC1', 'Campos ad hoc adicionais podem ser usados no seu modelo em locais em que quiser que apareçam. Para isso, edite o seu ficheiro entries.tpl e coloque etiquetas Smarty da forma {$entry.properties.ep_MyCustomField} no HTML onde desejar. Note o prefixo ep_ para cada campo. ');
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CUSTOMFIELDS_DESC2', 'Aqui pode introduzir uma lista de nomes de campos separados por vírgulas que podem ser usados para cada entrada - não use caracteres especiais ou espaços nesses nomes de campos. Exemplo: "Customfield1, Customfield2". ' . PLUGIN_EVENT_ENTRYPROPERTIES_CUSTOMFIELDS_DESC1);
@define('PLUGIN_EVENT_ENTRYPROPERTIES_CUSTOMFIELDS_DESC3', 'A lista de campos ad hoc disponíveis pode ser mudada na <a href="%s" target="_blank" title="' . PLUGIN_EVENT_ENTRYPROPERTIES_TITLE . '">configuração de plugins</a>.');
?>
