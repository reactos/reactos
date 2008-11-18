<?php # $Id: lang_ja.inc.php,v 1.4 2005/05/17 11:37:42 garvinhicking Exp $

##########################################################################
# serendipity - another blogger...                                       #
##########################################################################
#                                                                        #
# (c) 2003 Jannis Hermanns <J@hacked.it>                                 #
# http://www.jannis.to/programming/serendipity.html                      #
#                                                                        #
# Translated by                                                          #
# João P. Matos <jmatos@math.ist.utl.pt>                                 #
#                                                                        #
##########################################################################

@define('PLUGIN_EVENT_TEXTWIKI_NAME',     'Código: Wiki');
@define('PLUGIN_EVENT_TEXTWIKI_DESC',     'Codificação do texto usando Text_Wiki');
@define('PLUGIN_EVENT_TEXTWIKI_TRANSFORM', 'Síntaxe <a href="http://c2.com/cgi/wiki">Wiki</a> autorizada');

// 

@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_PREFILETER', 'Converte fins de linha de diferentes sistemas operativos (Unix/DOS) para um formato único e concatena linhas terminadas em \. Activado por omissão. É recomendado manter activo.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_DELIMITER', 'Converte o limitador interno do Text_Wiki "\xFF" (255) para evitar conflitos na interpretação. Activo por omissão. Recomendado manter activo.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_CODE', 'Marca texto texto entre <code> e </code> como código. Usando <code type=".."> pode activar formatação (e.g. para PHP). Activo por omissão.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_PHPCODE', 'Marca e formata texto entre <php> e </php> como código php e adiciona etiquetas abertas de PHP . Activo por omissão.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_HTML', 'Permite escrever código HTML entre <html> e </html>. Cuidado que JS também é possível! Se usar isto, não use codificação de comentários! Inactivo por omissão. Recomendado manter inactivo.'); // Review
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_RAW', 'Texto entre `` e `` não ´e interpretado por quaisquer outras regras. Activo por omissão.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_INCLUDE', 'Permite incluir e correr código PHP com a síntaxe [[include /caminho/para/script.php]]. O resultado é interpretado pelas regras de codificação. Cuidado, risco de segurança! Se usar isto, não use codificação de comentários! Inactivo por omissão. Recomendado manter inactivo.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_INCLUDE_DESC_BASE', 'O directório de base para os seus scripts. Por omissão "/caminho/para/scripts/". Se deixar em branco e ligar include só pode usar caminhos absolutos.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_HEADING', 'Linhas começando com "+ " são marcadas como títulos (+ = <h1>, ++++++ = <h6>). Activo por omissão.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_HORIZ', '---- é convertido para uma linha horizontal (<hr>). Activo por omissão.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_BREAK', 'Fins de linha marcados com " _" definem fins de linha explícitos. Activo por omissão.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_BLOCKQUOTE', 'Permite usar citações de tipo email ("> ", ">> ",...). Activo por omissão.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_LIST', 'Permite criação de listass ("* " = não numeradas, "# " = numeradas). Activo por omissão.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_DEFLIST', 'Permite criar listas de definições. Síntaxe: ": Tópico : Definição". Activo por omissão.'); //Verify
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_TABLE', 'permite criar tabelas. Só usar para linhas completas. Síntaxe: "|| Célula 1 || Célula 2 || Célula 3 ||". Activo por omissão.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_EMBED', 'Permite incluir e correr código PHP com a síntaxe [[embed /caminho/para/script.php]]. O resultado não é interpretado pelas regras de codificação. Cuidado, risco de segurança! Se usar isto, não use codificação de comentários! Inactivo por omissão. Recomendado manter inactivo.'); //Verify
@define('PLUGIN_EVENT_TEXTWIKI_RULE_EMBED_DESC_BASE', 'O directório de base para os seus scripts. Por omissão "/caminho/para/scripts/". Se deixar em branco e ligar embed só pode usar caminhos absolutos.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_IMAGE', 'Permite a inclusão de imagens. ([[image  /caminho/para/imagem.ext [atributos HTML] ]] or [[image  caminho/para/imagem.ext [link="NomePágina"] [atributos HTML] ]] para imagens com conexão). Default is on.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_IMAGE_DESC_BASE', 'O directório de base para as suas imagens. Por omissão "/caminho/para/imagens". Se deixar em branco só pode usar caminhos absolutos ou URLs.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_PHPLOOKUP', 'Cria ligações de busca ao manual de PHP com [[php function-name]]. Por omissão activo.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_TOC', 'Gera um índice de todos os títulos usados com [[toc]]. Por omissão activo.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_NEWLINE', 'Converte "newlines" ("\n") isoladas to line breakspara mudanças de linha. Por omissão activo.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_CENTER', 'Linhas começadas com "= " são centradas. Por omissão activo.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_PARAGRAPH', 'Converte "newlines" ("\n") duplas para parágrafos (<p></p>). Por omissão activo.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_URL', 'Converte http://example.com para ligação, [http://example.com] para nota de pé de página e [http://example.com Example] para uma ligação com descrição. Por omissão activo.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_URL_DESC_TARGET', 'Define o alvo (target) das suas URLs. Por omissão é "_blank".'); //Verify
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_FREELINK', 'permite definição de ligações não-standard de wiki via "((Non-standard link format))" e "((Non-standard link|Describtion))". Por omissão inactivo.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_FREELINK_DESC_PAGES', 'A regra de freelink (assim como a regra wikilink) devem saber que páginas existem e que páginas devem ser marcadas como "novas". Isto especifica a localização de um ficheiro (local ou remoto) que tem que conter 1 nome de página por linha. Se o ficheiro for remoto, será posto em cache pelo tempo especificado.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_FREELINK_DESC_VIEWURL', 'Esta URL é especificada para visualizar freelinks. Tem que especificar um "%s" dentro desta URL que será substituído pelo nome da página freelink.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_FREELINK_DESC_NEWURL', 'Esta URL é especificada para criar novas freelinks. Tem que especificar um "%s" dentro desta URL que será substituído pelo nome da página freelink.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_FREELINK_DESC_NEWTEXT', 'Este texto será adicionado a freelinks não definidas para ligar à página de criação. Incializado como "?".');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_FREELINK_DESC_CACHETIME', 'Se especificar um ficheiro remoto (URL) para as suas páginas de freelinks, este ficheiro estará em cache durante o número de segundos especificado aqui. O valor por omissão é de 1 hora.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_INTERWIKI', 'Permite ligações inter wiki a MeatBall, Advogato e Wiki usando SiteName:PageName ou [NomeSítio:NomePágina Mostrar este texto alternativo]. Activo por omissão.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_INTERWIKI_DESC_TARGET', '');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_WIKILINK', 'Permite uso de PalavrasWiki (WikiWords) standard (2-X x maiúsculas) como links wiki. por omissão inactivo.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_WIKILINK_DESC_PAGES', 'A regra wikilink deve saber que páginas existem e quais devem ser marcadas como "novas". Isto especifica a localização de um ficheiro (local ou remoto) que tem que conter um nome de página por linha. Se o ficheiro for remoto, será posto em cache pelo tempo especificado.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_WIKILINK_DESC_VIEWURL', 'A URL especificada para ver as wikilinks. Tem que especificar "%s" dentro desta URL que será substituído pelo nome da página wikilink.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_WIKILINK_DESC_NEWURL', 'Esta URL é especificada para criar novos wikilinks. Tem que especificar "%s" dentro desta URL que será substituído pelo nome da página wikilink.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_WIKILINK_DESC_NEWTEXT', 'Este texto será adicionado a wikilinks não definidas para ligar à página de criação. Incializado como "?".');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_WIKILINK_DESC_CACHETIME', 'Se especificar um ficheiro remoto (URL) para as suas páginas wikilink, este ficheiro estará em cache durante o número de segundos especificado aqui. O valor por omissão é de 1 hora.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_COLORTEXT', 'Colorir texto usando ##cor|texto##. Por omissão activo.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_STRONG', '**Texto** é marcado como forte (strong). Por omissão activo.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_BOLD', '\'\'\'Texto\'\'\' á marcado a negrito (bold). Por omissão activo.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_EMPHASIS', '//Texto// é marcado com ênfase. Por omissão activo.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_ITALIC', '\'\'Texto\'\' é marcado itálico. Por omissão activo.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_TT', '{{Texto}} é escrito com caracteres de teletipo (monotype). Por omissão activo.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_SUPERSCRIPT', '^^Texto^^ é escrito como superescrito. Por omissão activo.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_REVISE', 'Permite marcar texto como revisões usando "@@---texto a apagar+++texto a inserir@@". Por omissão activo.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_TIGHTEN', 'Encontra seuências de mais de 3 newlines e redu-las a 2 newlines (parágrafo). Por omissão activo.');
@define('PLUGIN_EVENT_TEXTWIKI_RULE_DESC_ENTITIES', 'Escapar entidades HTML. Por omissão activo.');


/* vim: set sts=4 ts=4 expandtab : */
?>