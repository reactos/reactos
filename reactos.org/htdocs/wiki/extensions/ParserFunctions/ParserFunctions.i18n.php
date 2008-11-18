<?php
/**
 * Internationalisation file for extension ParserFunctions.
 *
 * @addtogroup Extensions
*/

$messages = array();

$messages['en'] = array(
	'pfunc_desc'                            => 'Enhance parser with logical functions',
	'pfunc_time_error'                      => 'Error: invalid time',
	'pfunc_time_too_long'                   => 'Error: too many #time calls',
	'pfunc_rel2abs_invalid_depth'           => 'Error: Invalid depth in path: "$1" (tried to access a node above the root node)',
	'pfunc_expr_stack_exhausted'            => 'Expression error: Stack exhausted',
	'pfunc_expr_unexpected_number'          => 'Expression error: Unexpected number',
	'pfunc_expr_preg_match_failure'         => 'Expression error: Unexpected preg_match failure',
	'pfunc_expr_unrecognised_word'          => 'Expression error: Unrecognised word "$1"',
	'pfunc_expr_unexpected_operator'        => 'Expression error: Unexpected $1 operator',
	'pfunc_expr_missing_operand'            => 'Expression error: Missing operand for $1',
	'pfunc_expr_unexpected_closing_bracket' => 'Expression error: Unexpected closing bracket',
	'pfunc_expr_unrecognised_punctuation'   => 'Expression error: Unrecognised punctuation character "$1"',
	'pfunc_expr_unclosed_bracket'           => 'Expression error: Unclosed bracket',
	'pfunc_expr_division_by_zero'           => 'Division by zero',
	'pfunc_expr_invalid_argument'           => 'Invalid argument for $1: < -1 or > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Invalid argument for ln: <= 0',
	'pfunc_expr_unknown_error'              => 'Expression error: Unknown error ($1)',
	'pfunc_expr_not_a_number'               => 'In $1: result is not a number',
);

/** Afrikaans (Afrikaans)
 * @author Naudefj
 */
$messages['af'] = array(
	'pfunc_desc'                            => 'Verryk die ontleder met logiese funksies',
	'pfunc_time_error'                      => 'Fout: ongeldige tyd',
	'pfunc_time_too_long'                   => 'Fout: #time te veel kere geroep',
	'pfunc_rel2abs_invalid_depth'           => 'Fout: Ongeldige diepte in pad: "$1" (probeer \'n node bo die wortelnode te roep)',
	'pfunc_expr_stack_exhausted'            => 'Fout in uitdrukking: stack uitgeput',
	'pfunc_expr_unexpected_number'          => 'Fout in uitdrukking: onverwagte getal',
	'pfunc_expr_preg_match_failure'         => 'Fout in uitdrukking: onverwagte faling van preg_match',
	'pfunc_expr_unrecognised_word'          => 'Fout in uitdrukking: woord "$1" nie herken',
	'pfunc_expr_unexpected_operator'        => 'Fout in uitdrukking: onverwagte operateur $1',
	'pfunc_expr_missing_operand'            => 'Fout in uitdrukking: geen operand vir $1',
	'pfunc_expr_unexpected_closing_bracket' => 'Fout in uitdrukking: hakkie onverwags gesluit',
	'pfunc_expr_unrecognised_punctuation'   => 'Fout in uitdrukking: onbekende leesteken "$1"',
	'pfunc_expr_unclosed_bracket'           => 'Fout in uitdrukking: hakkie nie gesluit nie',
	'pfunc_expr_division_by_zero'           => 'Deling deur nul',
	'pfunc_expr_invalid_argument'           => 'Ongeldige argument vir $1: < -1 of > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Ongeldige argument vir ln: <= 0',
	'pfunc_expr_unknown_error'              => 'Fout in uitdrukking: onbekende fout ($1)',
	'pfunc_expr_not_a_number'               => "In $1: resultaat is nie 'n getal nie",
);

/** Aragonese (Aragonés)
 * @author Juanpabl
 */
$messages['an'] = array(
	'pfunc_desc'                            => 'Amillorar o parseyador con funzions lochicas',
	'pfunc_time_error'                      => 'Error: tiempo incorreuto',
	'pfunc_time_too_long'                   => 'Error: masiadas cridas #time',
	'pfunc_rel2abs_invalid_depth'           => 'Error: Fondura incorreuta en o path: "$1" (prebó d\'azeder ta un nodo por denzima d\'o nodo radiz)',
	'pfunc_expr_stack_exhausted'            => "Error d'espresión: Pila acotolada",
	'pfunc_expr_unexpected_number'          => "Error d'espresión: numbero no asperato",
	'pfunc_expr_preg_match_failure'         => "Error d'espresión: fallo de preg_match no asperato",
	'pfunc_expr_unrecognised_word'          => 'Error d\'espresión: palabra "$1" no reconoixita',
	'pfunc_expr_unexpected_operator'        => "Error d'espresión: operador $1 no asperato",
	'pfunc_expr_missing_operand'            => "Error d'espresión: á $1 li falta un operando",
	'pfunc_expr_unexpected_closing_bracket' => "Error d'espresión: zarradura d'o gafet no asperata",
	'pfunc_expr_unrecognised_punctuation'   => 'Error d\'espresión: caráuter de puntuazión "$1" no reconoixito',
	'pfunc_expr_unclosed_bracket'           => "Error d'espresión: gafet sin zarrar",
	'pfunc_expr_division_by_zero'           => 'Dibisión por zero',
	'pfunc_expr_invalid_argument'           => 'Argumento no conforme ta $1: < -1 u > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Argumento no conforme ta ln: <=0',
	'pfunc_expr_unknown_error'              => "Error d'espresión: error esconoixito ($1)",
	'pfunc_expr_not_a_number'               => 'En $1: o resultau no ye un numero',
);

/** Arabic (العربية)
 * @author Meno25
 * @author Siebrand
 */
$messages['ar'] = array(
	'pfunc_desc'                            => 'بارسر ممدد بدوال منطقية',
	'pfunc_time_error'                      => 'خطأ: زمن غير صحيح',
	'pfunc_time_too_long'                   => 'خطأ: too many #time calls',
	'pfunc_rel2abs_invalid_depth'           => 'خطأ: عمق غير صحيح في المسار: "$1" (حاول دخول عقدة فوق العقدة الجذرية)',
	'pfunc_expr_stack_exhausted'            => 'خطأ في التعبير: ستاك مجهد',
	'pfunc_expr_unexpected_number'          => 'خطأ في التعبير: رقم غير متوقع',
	'pfunc_expr_preg_match_failure'         => 'خطأ في التعبير: فشل preg_match غير متوقع',
	'pfunc_expr_unrecognised_word'          => 'خطأ في التعبير: كلمة غير متعرف عليها "$1"',
	'pfunc_expr_unexpected_operator'        => 'خطأ في التعبير: عامل $1 غير متوقع',
	'pfunc_expr_missing_operand'            => 'خطأ في التعبير: operand مفقود ل$1',
	'pfunc_expr_unexpected_closing_bracket' => 'خطأ في التعبير: قوس إغلاق غير متوقع',
	'pfunc_expr_unrecognised_punctuation'   => 'خطأ في التعبير: علامة ترقيم غير متعرف عليها "$1"',
	'pfunc_expr_unclosed_bracket'           => 'خطأ في التعبير: قوس غير مغلق',
	'pfunc_expr_division_by_zero'           => 'القسمة على صفر',
	'pfunc_expr_invalid_argument'           => 'مدخلة غير صحيحة ل $1: < -1 أو > 1',
	'pfunc_expr_invalid_argument_ln'        => 'مدخلة غير صحيحة ل ln: <= 0',
	'pfunc_expr_unknown_error'              => 'خطأ في التعبير: خطأ غير معروف ($1)',
	'pfunc_expr_not_a_number'               => 'في $1: النتيجة ليست رقما',
);

/** Assamese (অসমীয়া)
 * @author Rajuonline
 */
$messages['as'] = array(
	'pfunc_time_error' => 'ভুল: অযোগ্য সময়',
);

/** Asturian (Asturianu)
 * @author Esbardu
 */
$messages['ast'] = array(
	'pfunc_time_error'                      => 'Error: tiempu non válidu',
	'pfunc_time_too_long'                   => 'Error: demasiaes llamaes #time',
	'pfunc_rel2abs_invalid_depth'           => 'Error: Nivel de subdireutoriu non válidu: "$1" (intentu d\'accesu penriba del direutoriu raíz)',
	'pfunc_expr_stack_exhausted'            => "Error d'espresión: Pila escosada",
	'pfunc_expr_unexpected_number'          => "Error d'espresión: Númberu inesperáu",
	'pfunc_expr_preg_match_failure'         => "Error d'espresión: Fallu inesperáu de preg_match",
	'pfunc_expr_unrecognised_word'          => 'Error d\'espresión: Pallabra "$1" non reconocida',
	'pfunc_expr_unexpected_operator'        => "Error d'espresión: Operador $1 inesperáu",
	'pfunc_expr_missing_operand'            => "Error d'espresión: Falta operador en $1",
	'pfunc_expr_unexpected_closing_bracket' => "Error d'espresión: Paréntesis final inesperáu",
	'pfunc_expr_unrecognised_punctuation'   => 'Error d\'espresión: Caráuter de puntuación "$1" non reconocíu',
	'pfunc_expr_unclosed_bracket'           => "Error d'espresión: Paréntesis non zarráu",
	'pfunc_expr_division_by_zero'           => 'División por cero',
	'pfunc_expr_invalid_argument'           => 'Argumentu non válidu pa $1: < -1 o > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Argumentu non válidu pa ln: <= 0',
	'pfunc_expr_unknown_error'              => "Error d'espresión: Error desconocíu ($1)",
	'pfunc_expr_not_a_number'               => 'En $1: el resultáu nun ye un númberu',
);

/** Southern Balochi (بلوچی مکرانی)
 * @author Mostafadaneshvar
 */
$messages['bcc'] = array(
	'pfunc_desc'                            => 'تجزیه کنوکء بهتر کن گون عملگر آن منطقی',
	'pfunc_time_error'                      => 'حطا: نامعتبر وهد',
	'pfunc_time_too_long'                   => 'حطا: بازگین #زمان سوج',
	'pfunc_rel2abs_invalid_depth'           => 'حطا: نامعتبر عمق ته مسیر: "$1"(سعی کتن په یک بالادی گرهنی چه ریشگی گرهنانا برسیت)',
	'pfunc_expr_stack_exhausted'            => 'حطا اصطلاح: توده حالیک',
	'pfunc_expr_unexpected_number'          => 'حطا اصطلاح: غیر منظرین شماره',
	'pfunc_expr_preg_match_failure'         => 'حطا اصطلاح: غیرمنتظره این preg_ همسانی پروش وارت',
	'pfunc_expr_unrecognised_word'          => 'حطا اصطلاح: نا شناسین کلمه  "$1"',
	'pfunc_expr_unexpected_operator'        => 'حطا اصطلاح:نه لوٹتین  $1 اپراتور',
	'pfunc_expr_missing_operand'            => 'حطا اصطلاح: گارین عملوند په $1',
	'pfunc_expr_unexpected_closing_bracket' => 'حطا اصطلاح: نه لوٹتگین براکت بندگ',
	'pfunc_expr_unrecognised_punctuation'   => 'حطا اصطلاح: ناشناسین کاراکتر نشانه هلگی "$1"',
	'pfunc_expr_unclosed_bracket'           => 'حطا اصطلاح: نه بسته گین براکت',
	'pfunc_expr_division_by_zero'           => 'تقسیم بر صفر',
	'pfunc_expr_invalid_argument'           => 'نامعتبر آرگومان په  $1: < -1 یا > 1',
	'pfunc_expr_invalid_argument_ln'        => 'نامعتبر آرگومان ته شی : <= 0',
	'pfunc_expr_unknown_error'              => 'حطا اصطلاح :ناشناسین حطا ($1)',
	'pfunc_expr_not_a_number'               => 'ته $1: نتیجه یک عددی نهنت',
);

/** Belarusian (Taraškievica orthography) (Беларуская (тарашкевіца))
 * @author EugeneZelenko
 */
$messages['be-tarask'] = array(
	'pfunc_time_error' => 'Памылка: няслушны час',
);

/** Bulgarian (Български)
 * @author Spiritia
 * @author DCLXVI
 */
$messages['bg'] = array(
	'pfunc_desc'                            => 'Подобрвяване на парсера с логически функции',
	'pfunc_time_error'                      => 'Грешка: невалидно време',
	'pfunc_time_too_long'                   => 'Грешка: Твърде много извиквания на #time',
	'pfunc_expr_stack_exhausted'            => 'Грешка в записа: Стекът е изчерпан',
	'pfunc_expr_unexpected_number'          => 'Грешка в записа: Неочаквано число',
	'pfunc_expr_unrecognised_word'          => 'Грешка в записа: Неразпозната дума "$1"',
	'pfunc_expr_unexpected_operator'        => 'Грешка в записа: Неочакван оператор $1',
	'pfunc_expr_missing_operand'            => 'Грешка в записа: Липсващ операнд в $1',
	'pfunc_expr_unexpected_closing_bracket' => 'Грешка в записа: Една затваряща скоба в повече',
	'pfunc_expr_unrecognised_punctuation'   => 'Грешка в записа: Неразпознат пунктуационен знак "$1"',
	'pfunc_expr_unclosed_bracket'           => 'Грешка в записа: Незатворена скоба',
	'pfunc_expr_division_by_zero'           => 'Деление на нула',
	'pfunc_expr_unknown_error'              => 'Грешка в записа: Неразпозната грешка ($1)',
	'pfunc_expr_not_a_number'               => 'В $1: резултатът не е число',
);

/** Bengali (বাংলা)
 * @author Zaheen
 * @author Bellayet
 */
$messages['bn'] = array(
	'pfunc_desc'                            => 'লজিকাল ফাংশন দিয়ে পার্সারকে উন্নত করুন',
	'pfunc_time_error'                      => 'ত্রুটি: অবৈধ সময়',
	'pfunc_time_too_long'                   => 'ত্রুটি: অত্যধিক সংখ্যক #time কল',
	'pfunc_rel2abs_invalid_depth'           => 'ত্রুটি: পাথে অবৈধ গভীরতা: "$1" (মূল নোডের উপরের একটি নোড অ্যাক্সেস করতে চেষ্টা করেছিল)',
	'pfunc_expr_stack_exhausted'            => 'এক্সপ্রেশন ত্রুটি: স্ট্যাক শেষ হয়ে গেছে',
	'pfunc_expr_unexpected_number'          => 'এক্সপ্রেশন ত্রুটি: অযাচিত সংখ্যা',
	'pfunc_expr_preg_match_failure'         => 'এক্সপ্রেশন ত্রুটি: অযাচিত preg_match ব্যর্থতা',
	'pfunc_expr_unrecognised_word'          => 'এক্সপ্রেশন ত্রুটি: অপরিচিত শব্দ "$1"',
	'pfunc_expr_unexpected_operator'        => 'এক্সপ্রেশন ত্রুটি: অযাচিত $1 অপারেটর',
	'pfunc_expr_missing_operand'            => 'এক্সপ্রেশন ত্রুটি: $1-এর জন্য অপারেন্ড নেই।',
	'pfunc_expr_unexpected_closing_bracket' => 'এক্সপ্রেশন ত্রুটি: অযাচিত সমাপ্তকারী বন্ধনী',
	'pfunc_expr_unrecognised_punctuation'   => 'এক্সপ্রেশন ত্রুটি: অপরিচিত বিরামচিহ্ন ক্যারেক্টার "$1"',
	'pfunc_expr_unclosed_bracket'           => 'এক্সপ্রেশন ত্রুটি: উন্মুক্ত বন্ধনী',
	'pfunc_expr_division_by_zero'           => 'শূন্য দ্বারা ভাগ করা হয়েছে',
	'pfunc_expr_unknown_error'              => 'এক্সপ্রেশন ত্রুটি: অজানা ত্রুটি ($1)',
	'pfunc_expr_not_a_number'               => '$1: এ ফলাফল কোন সংখ্যা নয়',
);

/** Breton (Brezhoneg)
 * @author Fulup
 */
$messages['br'] = array(
	'pfunc_desc'                            => "Barrekaat a ra ar parser gant arc'hwelioù poellek.",
	'pfunc_time_error'                      => 'Fazi : pad direizh',
	'pfunc_time_too_long'                   => 'Fazi : betek re eo bet galvet #time',
	'pfunc_rel2abs_invalid_depth'           => "Fazi : Donder direizh evit an hent : \"\$1\" (klasket ez eus bet mont d'ul live a-us d'ar c'havlec'h-mamm)",
	'pfunc_expr_stack_exhausted'            => 'Kemennad faziek : pil riñset',
	'pfunc_expr_unexpected_number'          => "Kemennad faziek : niver dic'hortoz",
	'pfunc_expr_preg_match_failure'         => "Kemennad faziek : c'hwitadenn dic'hortoz evit <code>preg_match</code>",
	'pfunc_expr_unrecognised_word'          => 'Kemennad faziek : Ger dianav "$1"',
	'pfunc_expr_unexpected_operator'        => 'Kemennad faziek : Oberier $1 dianav',
	'pfunc_expr_missing_operand'            => 'Kemennad faziek : Dianav eo operand $1',
	'pfunc_expr_unexpected_closing_bracket' => "Kemennad faziek : Krommell zehoù dic'hortoz",
	'pfunc_expr_unrecognised_punctuation'   => 'Kemennad faziek : arouezenn boentadouiñ dianav "$1"',
	'pfunc_expr_unclosed_bracket'           => 'Kemennad faziek : Krommell zigor',
	'pfunc_expr_division_by_zero'           => 'Rannañ dre mann',
	'pfunc_expr_invalid_argument'           => 'Talvoudenn direizh evit $1: < -1 pe > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Talvoudenn direizh evit ln: <= 0',
	'pfunc_expr_unknown_error'              => 'Kemennad faziek : Fazi dianav ($1)',
	'pfunc_expr_not_a_number'               => "E $1: An disoc'h n'eo ket un niver",
);

/** Catalan (Català)
 * @author SMP
 * @author Jordi Roqué
 */
$messages['ca'] = array(
	'pfunc_time_error'                      => 'Error: temps invàlid',
	'pfunc_time_too_long'                   => 'Error: massa crides #time',
	'pfunc_rel2abs_invalid_depth'           => "Error: Adreça invàlida al directori: «$1» (s'intentava accedir a un node superior de l'arrel)",
	'pfunc_expr_stack_exhausted'            => "Error de l'expressió: Pila exhaurida",
	'pfunc_expr_unexpected_number'          => "Error de l'expressió: Nombre inesperat",
	'pfunc_expr_preg_match_failure'         => "Error de l'expressió: Error de funció no compresa i inesperada",
	'pfunc_expr_unrecognised_word'          => 'Error de l\'expressió: Paraula no reconeguda "$1"',
	'pfunc_expr_unexpected_operator'        => "Error de l'expressió: Operador $1 inesperat",
	'pfunc_expr_missing_operand'            => "Error de l'expressió: Falta l'operand de $1",
	'pfunc_expr_unexpected_closing_bracket' => "Error de l'expressió: Parèntesi inesperat",
	'pfunc_expr_unrecognised_punctuation'   => 'Error de l\'expressió: Signe de puntuació no reconegut "$1"',
	'pfunc_expr_unclosed_bracket'           => "Error de l'expressió: Parèntesi no tancat",
	'pfunc_expr_division_by_zero'           => 'Divisió entre zero',
	'pfunc_expr_invalid_argument'           => 'Valor no vàlid per a $1: < -1 ó > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Valor no vàlid per a ln: <= 0',
	'pfunc_expr_unknown_error'              => "Error de l'expressió: Desconegut ($1)",
	'pfunc_expr_not_a_number'               => 'A $1: el resultat no és un nombre',
);

/** Czech (Česky)
 * @author Li-sung
 * @author Danny B.
 * @author Sp5uhe
 * @author Siebrand
 * @author Matěj Grabovský
 */
$messages['cs'] = array(
	'pfunc_desc'                            => 'Rozšíření parseru o logické funkce',
	'pfunc_time_error'                      => 'Chyba: neplatný čas',
	'pfunc_time_too_long'                   => 'Chyba: příliš mnoho volání #time',
	'pfunc_rel2abs_invalid_depth'           => 'Chyba: Neplatná hloubka v cestě: "$1" (pokus o přístup do uzlu vyššího než kořen)',
	'pfunc_expr_stack_exhausted'            => 'Chyba ve výrazu: Zásobník plně obsazen',
	'pfunc_expr_unexpected_number'          => 'Chyba ve výrazu: Očekáváno číslo',
	'pfunc_expr_preg_match_failure'         => 'Chyba ve výrazu: Neočekávaná chyba funkce preg_match',
	'pfunc_expr_unrecognised_word'          => 'Chyba ve výrazu: Nerozpoznané slovo „$1“',
	'pfunc_expr_unexpected_operator'        => 'Chyba ve výrazu: Neočekávaný operátor $1',
	'pfunc_expr_missing_operand'            => 'Chyba ve výrazu: Chybí operand pro $1',
	'pfunc_expr_unexpected_closing_bracket' => 'Chyba ve výrazu: Neočekávaná uzavírací závorka',
	'pfunc_expr_unrecognised_punctuation'   => 'Chyba ve výrazu: Nerozpoznaný interpunkční znak „$1“',
	'pfunc_expr_unclosed_bracket'           => 'Chyba ve výrazu: Neuzavřené závorky',
	'pfunc_expr_division_by_zero'           => 'Dělení nulou',
	'pfunc_expr_invalid_argument'           => 'Neplatný argument pro $1: < -1 nebo > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Neplatný argument pro ln: <= 0',
	'pfunc_expr_unknown_error'              => 'Chyba ve výrazu: Neznámá chyba ($1)',
	'pfunc_expr_not_a_number'               => 'V $1: výsledkem není číslo',
);

/** Danish (Dansk)
 * @author Morten
 */
$messages['da'] = array(
	'pfunc_desc'                            => 'Udvidet parser med logiske funktioner',
	'pfunc_time_error'                      => 'Fejl: Ugyldig tid',
	'pfunc_time_too_long'                   => 'Felj: for mange #time kald',
	'pfunc_expr_stack_exhausted'            => 'Udtryksfejl: Stack tømt',
	'pfunc_expr_unexpected_number'          => 'Fejl: Uventet nummer',
	'pfunc_expr_preg_match_failure'         => 'Udtryksfejl: Uventet preg_match fejl',
	'pfunc_expr_unrecognised_word'          => 'Udtryksfejl: Uventet ord "$1"',
	'pfunc_expr_unexpected_operator'        => 'Udtryksfejl: Uventet $1 operator',
	'pfunc_expr_missing_operand'            => 'Udtryksfejl: Manglende operand til $1',
	'pfunc_expr_unexpected_closing_bracket' => 'Udtryksfejl: Uventet "]"-tegn',
	'pfunc_expr_unrecognised_punctuation'   => 'Udtryksfejl: Uventet tegnsætning-tegn: "$1"',
	'pfunc_expr_unclosed_bracket'           => 'Udtryksfejl: Uafsluttet kantet parantes',
	'pfunc_expr_division_by_zero'           => 'Division med nul',
	'pfunc_expr_unknown_error'              => 'Udtryksfejl: Ukendt fejl ($1)',
	'pfunc_expr_not_a_number'               => 'I $1: Resultatet er ikke et tal',
);

/** German (Deutsch)
 * @author Raimond Spekking
 */
$messages['de'] = array(
	'pfunc_desc'                            => 'Erweitert den Parser um logische Funktionen',
	'pfunc_time_error'                      => 'Fehler: ungültige Zeitangabe',
	'pfunc_time_too_long'                   => 'Fehler: zu viele #time-Aufrufe',
	'pfunc_rel2abs_invalid_depth'           => 'Fehler: ungültige Tiefe in Pfad: „$1“ (Versuch, auf einen Knotenpunkt oberhalb des Hauptknotenpunktes zuzugreifen)',
	'pfunc_expr_stack_exhausted'            => 'Expression-Fehler: Stacküberlauf',
	'pfunc_expr_unexpected_number'          => 'Expression-Fehler: Unerwartete Zahl',
	'pfunc_expr_preg_match_failure'         => 'Expression-Fehler: Unerwartete „preg_match“-Fehlfunktion',
	'pfunc_expr_unrecognised_word'          => 'Expression-Fehler: Unerkanntes Wort „$1“',
	'pfunc_expr_unexpected_operator'        => 'Expression-Fehler: Unerwarteter Operator: <tt>$1</tt>',
	'pfunc_expr_missing_operand'            => 'Expression-Fehler: Fehlender Operand für <tt>$1</tt>',
	'pfunc_expr_unexpected_closing_bracket' => 'Expression-Fehler: Unerwartete schließende eckige Klammer',
	'pfunc_expr_unrecognised_punctuation'   => 'Expression-Fehler: Unerkanntes Satzzeichen „$1“',
	'pfunc_expr_unclosed_bracket'           => 'Expression-Fehler: Nicht geschlossene eckige Klammer',
	'pfunc_expr_division_by_zero'           => 'Expression-Fehler: Division durch Null',
	'pfunc_expr_invalid_argument'           => 'Ungültiges Argument für $1: < -1 oder > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Ungültiges Argument für ln: <= 0',
	'pfunc_expr_unknown_error'              => 'Expression-Fehler: Unbekannter Fehler ($1)',
	'pfunc_expr_not_a_number'               => 'Expression-Fehler: In $1: Ergebnis ist keine Zahl',
);

/** Greek (Ελληνικά)
 * @author Απεργός
 * @author Consta
 */
$messages['el'] = array(
	'pfunc_time_error'    => 'Σφάλμα: άκυρος χρόνος',
	'pfunc_time_too_long' => 'Σφάλμα: πάρα πολλές κλήσεις της #time',
);

/** Esperanto (Esperanto)
 * @author Yekrats
 */
$messages['eo'] = array(
	'pfunc_desc'                            => 'Etendu sintaksan analizilon kun logikaj funkcioj',
	'pfunc_time_error'                      => 'Eraro: nevalida tempo',
	'pfunc_time_too_long'                   => "Eraro: tro da vokoj ''#time''",
	'pfunc_rel2abs_invalid_depth'           => 'Eraro: Nevalida profundo en vojo: "$1" (provis atingi nodon super la radika nodo)',
	'pfunc_expr_stack_exhausted'            => 'Esprima eraro: Stako estis malplenigita',
	'pfunc_expr_unexpected_number'          => 'Esprima eraro: Neatendita numeralo',
	'pfunc_expr_preg_match_failure'         => 'Esprima eraro: Neatendita preg_match malsukceso',
	'pfunc_expr_unrecognised_word'          => 'Esprima eraro: Nekonata vorto "$1"',
	'pfunc_expr_unexpected_operator'        => 'Esprima eraro: Neatendita operacisimbolo $1',
	'pfunc_expr_missing_operand'            => 'Esprima eraro: Mankas operando por $1',
	'pfunc_expr_unexpected_closing_bracket' => 'Esprima eraro: Neatendita ferma krampo',
	'pfunc_expr_unrecognised_punctuation'   => 'Esprima eraro: Nekonata interpunkcia simbolo "$1"',
	'pfunc_expr_unclosed_bracket'           => 'Esprima eraro: Malferma krampo',
	'pfunc_expr_division_by_zero'           => 'Divido per nulo',
	'pfunc_expr_invalid_argument'           => 'Nevalida argumento por $1: < -1 or > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Nevalida argumento por ln: <= 0',
	'pfunc_expr_unknown_error'              => 'Esprima eraro: Nekonata eraro ($1)',
	'pfunc_expr_not_a_number'               => 'En $1: rezulto ne estas nombro',
);

/** Basque (Euskara)
 * @author SPQRobin
 */
$messages['eu'] = array(
	'pfunc_time_error'            => 'Errorea: baliogabeko ordua',
	'pfunc_time_too_long'         => 'Errorea: #time dei gehiegi',
	'pfunc_rel2abs_invalid_depth' => 'Errorea: Baliogabeko sakonera fitxategi bidean: "$1" (root puntutik gora sartzen saiatu da)',
);

/** Persian (فارسی)
 * @author Huji
 */
$messages['fa'] = array(
	'pfunc_desc'                            => 'به تجزیه‌گر، دستورهای منطقی می‌افزاید',
	'pfunc_time_error'                      => 'خطا: زمان غیرمجاز',
	'pfunc_time_too_long'                   => 'خطا: فراخوانی بیش از حد #time',
	'pfunc_rel2abs_invalid_depth'           => 'خطا: عمق غیر مجاز در نشانی «$1» (تلاش برای دسترسی به یک نشانی فراتر از نشانی ریشه)',
	'pfunc_expr_stack_exhausted'            => 'خطای عبارت: پشته از دست رفته',
	'pfunc_expr_unexpected_number'          => 'خطای عبارت: عدد دور از انتظار',
	'pfunc_expr_preg_match_failure'         => 'خطای عبارت: خطای preg_match دور از انتظار',
	'pfunc_expr_unrecognised_word'          => 'خطای عبارت: کلمه ناشناخته «$1»',
	'pfunc_expr_unexpected_operator'        => 'خطای عبارت: عملگر $1 دور از انتظار',
	'pfunc_expr_missing_operand'            => 'خطای عبارت: عملگر گمشده برای $1',
	'pfunc_expr_unexpected_closing_bracket' => 'خطای عبارت: پرانتز بسته اضافی',
	'pfunc_expr_unrecognised_punctuation'   => 'خطای عبارت: نویسه نقطه‌گذاری شناخته نشده «$1»',
	'pfunc_expr_unclosed_bracket'           => 'خطای عبارت: پرانتز بسته‌نشده',
	'pfunc_expr_division_by_zero'           => 'تقسیم بر صفر',
	'pfunc_expr_invalid_argument'           => 'پارامتر غیر مجاز برای $1: < -۱ یا > ۱',
	'pfunc_expr_invalid_argument_ln'        => 'پارامتر غیر مجاز برای لگاریتم طبیعی: <= صفر',
	'pfunc_expr_unknown_error'              => 'خطای عبارت: خطای ناشناخته ($1)',
	'pfunc_expr_not_a_number'               => 'در $1: نتیجه عدد نیست',
);

/** Finnish (Suomi)
 * @author Nike
 */
$messages['fi'] = array(
	'pfunc_desc'                            => 'Laajentaa jäsennintä loogisilla funktiolla.',
	'pfunc_time_error'                      => 'Virhe: kelvoton aika',
	'pfunc_time_too_long'                   => 'Virhe: liian monta #time-kutsua',
	'pfunc_expr_stack_exhausted'            => 'Virhe lausekkeessa: pino loppui',
	'pfunc_expr_unexpected_number'          => 'Virhe lausekkeessa: odottamaton numero',
	'pfunc_expr_preg_match_failure'         => 'Virhe lausekkeessa: <tt>preg_match</tt> palautti virheen',
	'pfunc_expr_unrecognised_word'          => 'Virhe lausekkeessa: tunnistamaton sana ”$1”',
	'pfunc_expr_unexpected_operator'        => 'Virhe lausekkeessa: odottamaton $1-operaattori',
	'pfunc_expr_missing_operand'            => 'Virhe lausekkeessa: operaattorin $1 edellyttämä operandi puuttuu',
	'pfunc_expr_unexpected_closing_bracket' => 'Virhe lausekkeessa: odottamaton sulkeva sulkumerkki',
	'pfunc_expr_unrecognised_punctuation'   => 'Virhe lausekkeessa: tunnistamaton välimerkki ”$1”',
	'pfunc_expr_unclosed_bracket'           => 'Virhe ilmauksessa: sulkeva sulkumerkki puuttuu',
	'pfunc_expr_division_by_zero'           => 'Virhe: Jako nollalla',
	'pfunc_expr_unknown_error'              => 'Virhe lausekkeessa: tuntematon virhe ($1)',
);

/** French (Français)
 * @author Grondin
 * @author Sherbrooke
 * @author Urhixidur
 */
$messages['fr'] = array(
	'pfunc_desc'                            => 'Augmente le parseur avec des fonctions logiques',
	'pfunc_time_error'                      => 'Erreur : durée invalide',
	'pfunc_time_too_long'                   => 'Erreur : <code>#time</code> appelé trop de fois',
	'pfunc_rel2abs_invalid_depth'           => 'Erreur: niveau de répertoire invalide dans le chemin : « $1 » (a essayé d’accéder à un niveau au-dessus du répertoire racine)',
	'pfunc_expr_stack_exhausted'            => 'Expression erronée : pile épuisée',
	'pfunc_expr_unexpected_number'          => 'Expression erronée : nombre inattendu',
	'pfunc_expr_preg_match_failure'         => 'Expression erronée : échec inattendu pour <code>preg_match</code>',
	'pfunc_expr_unrecognised_word'          => "Erreur d'expression : le mot '''$1''' n'est pas reconnu",
	'pfunc_expr_unexpected_operator'        => "Erreur d'expression : l'opérateur '''$1''' n'est pas reconnu",
	'pfunc_expr_missing_operand'            => "Erreur d’expression : l’opérande '''$1''' n’est pas reconnue",
	'pfunc_expr_unexpected_closing_bracket' => 'Erreur d’expression : parenthèse fermante inattendue',
	'pfunc_expr_unrecognised_punctuation'   => "Erreur d'expression : caractère de ponctuation « $1 » non reconnu",
	'pfunc_expr_unclosed_bracket'           => 'Erreur d’expression : parenthèse non fermée',
	'pfunc_expr_division_by_zero'           => 'Division par zéro',
	'pfunc_expr_invalid_argument'           => 'Valeur incorrecte pour $1 : < -1 ou > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Valeur incorrecte pour ln : ≤ 0',
	'pfunc_expr_unknown_error'              => "Erreur d'expression : erreur inconnue ($1)",
	'pfunc_expr_not_a_number'               => "Dans $1 : le résultat n'est pas un nombre",
);

/** Franco-Provençal (Arpetan)
 * @author ChrisPtDe
 */
$messages['frp'] = array(
	'pfunc_desc'                            => 'Ôgmente lo parsor avouéc des fonccions logiques.',
	'pfunc_time_error'                      => 'Èrror : durâ envalida',
	'pfunc_time_too_long'                   => 'Èrror : parsèr #time apelâ trop de côps',
	'pfunc_rel2abs_invalid_depth'           => 'Èrror : nivô de rèpèrtouèro envalido dens lo chemin : « $1 » (at tâchiê d’arrevar a un nivô en-dessus du rèpèrtouèro racena)',
	'pfunc_expr_stack_exhausted'            => 'Èxprèssion fôssa : pila èpouesiê',
	'pfunc_expr_unexpected_number'          => 'Èxprèssion fôssa : nombro emprèvu',
	'pfunc_expr_preg_match_failure'         => 'Èxprèssion fôssa : falyita emprèvua por <code>preg_match</code>',
	'pfunc_expr_unrecognised_word'          => "Èrror d’èxprèssion : lo mot '''$1''' est pas recognu",
	'pfunc_expr_unexpected_operator'        => "Èrror d’èxprèssion : l’opèrator '''$1''' est pas recognu",
	'pfunc_expr_missing_operand'            => "Èrror d’èxprèssion : l’opèranda '''$1''' est pas recognua",
	'pfunc_expr_unexpected_closing_bracket' => 'Èrror d’èxprèssion : parentèsa cllosenta emprèvua',
	'pfunc_expr_unrecognised_punctuation'   => 'Èrror d’èxprèssion : caractèro de ponctuacion « $1 » pas recognu',
	'pfunc_expr_unclosed_bracket'           => 'Èrror d’èxprèssion : parentèsa pas cllôsa',
	'pfunc_expr_division_by_zero'           => 'Division per zérô',
	'pfunc_expr_unknown_error'              => 'Èrror d’èxprèssion : èrror encognua ($1)',
	'pfunc_expr_not_a_number'               => 'Dens $1 : lo rèsultat est pas un nombro',
);

/** Galician (Galego)
 * @author Xosé
 * @author Toliño
 * @author Alma
 * @author Siebrand
 */
$messages['gl'] = array(
	'pfunc_desc'                            => 'Mellora o analizador con funcións lóxicas',
	'pfunc_time_error'                      => 'Erro: hora non válida',
	'pfunc_time_too_long'                   => 'Erro: demasiadas chamadas a #time',
	'pfunc_rel2abs_invalid_depth'           => 'Erro: Profundidade da ruta non válida: "$1" (tentouse acceder a un nodo por riba do nodo raíz)',
	'pfunc_expr_stack_exhausted'            => 'Erro de expresión: Pila esgotada',
	'pfunc_expr_unexpected_number'          => 'Erro de expresión: Número inesperado',
	'pfunc_expr_preg_match_failure'         => 'Erro de expresión: Fallo de preg_match inesperado',
	'pfunc_expr_unrecognised_word'          => 'Erro de expresión: Palabra descoñecida "$1"',
	'pfunc_expr_unexpected_operator'        => 'Erro de expresión: Operador $1 inesperado',
	'pfunc_expr_missing_operand'            => 'Erro de expresión: falta un operador para $1',
	'pfunc_expr_unexpected_closing_bracket' => 'Expresión de erro: Inesperado corchete',
	'pfunc_expr_unrecognised_punctuation'   => 'Erro de expresión: Signo de puntuación descoñecido "$1"',
	'pfunc_expr_unclosed_bracket'           => 'Erro de expresión: paréntese sen pechar',
	'pfunc_expr_division_by_zero'           => 'División por cero',
	'pfunc_expr_invalid_argument'           => 'Argumento inválido para $1: < -1 ou > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Argumento inválido para ln: <= 0',
	'pfunc_expr_unknown_error'              => 'Erro de expresión: Erro descoñecido ($1)',
	'pfunc_expr_not_a_number'               => 'En $1: o resultado non é un número',
);

/** Hebrew (עברית) */
$messages['he'] = array(
	'pfunc_desc'                            => 'הוספת פונקציות לוגיות למפענח',
	'pfunc_time_error'                      => 'שגיאה: זמן שגוי',
	'pfunc_time_too_long'                   => 'שגיאה: שימוש ב"#זמן" פעמים רבות מדי',
	'pfunc_rel2abs_invalid_depth'           => 'שגיאה: עומק שגוי בנתיב: "$1" (ניסיון כניסה לצומת מעל צומת השורש)',
	'pfunc_expr_stack_exhausted'            => 'שגיאה בביטוי: המחסנית מלאה',
	'pfunc_expr_unexpected_number'          => 'שגיאה בביטוי: מספר בלתי צפוי',
	'pfunc_expr_preg_match_failure'         => 'שגיאה בביטוי: כישלון בלתי צפוי של התאמת ביטוי רגולרי',
	'pfunc_expr_unrecognised_word'          => 'שגיאה בביטוי: מילה בלתי מזוהה, "$1"',
	'pfunc_expr_unexpected_operator'        => 'שגיאה בביטוי: אופרנד $1 בלתי צפוי',
	'pfunc_expr_missing_operand'            => 'שגיאה בביטוי: חסר אופרנד ל־$1',
	'pfunc_expr_unexpected_closing_bracket' => 'שגיאה בביטוי: סוגריים סוגרים בלתי צפויים',
	'pfunc_expr_unrecognised_punctuation'   => 'שגיאה בביטוי: תו פיסוק בלתי מזוהה, "$1"',
	'pfunc_expr_unclosed_bracket'           => 'שגיאה בביטוי: סוגריים בלתי סגורים',
	'pfunc_expr_division_by_zero'           => 'חלוקה באפס',
	'pfunc_expr_invalid_argument'           => 'ארגומנט בלתי תקין לפונקציה $1: < -1 או > 1',
	'pfunc_expr_invalid_argument_ln'        => 'ארגומנט בלתי תקין לפונקציה ln: <= 0',
	'pfunc_expr_unknown_error'              => 'שגיאה בביטוי: שגיאה בלתי ידועה ($1)',
	'pfunc_expr_not_a_number'               => 'התוצאה של $1 אינה מספר',
);

/** Hindi (हिन्दी)
 * @author Kaustubh
 * @author Shyam
 */
$messages['hi'] = array(
	'pfunc_desc'                            => 'लॉजिकल कार्योंका इस्तेमाल करके पार्सर बढायें',
	'pfunc_time_error'                      => 'गलती: गलत समय',
	'pfunc_time_too_long'                   => 'गलती: बहुत सारे #time कॉल',
	'pfunc_rel2abs_invalid_depth'           => 'गलती: पाथ में गलत गहराई: "$1" (रूट नोडके उपर वाले नोड खोजने की कोशीश की)',
	'pfunc_expr_stack_exhausted'            => 'एक्स्प्रेशनमें गलती: स्टॅक खतम हो गया',
	'pfunc_expr_unexpected_number'          => 'एक्स्प्रेशनमें गलती: अनपेक्षित संख्या',
	'pfunc_expr_preg_match_failure'         => 'एक्स्प्रेशन गलती: अनपेक्षित preg_match रद्दीकरण',
	'pfunc_expr_unrecognised_word'          => 'एक्स्प्रेशन गलती: अनिश्चित शब्द "$1"',
	'pfunc_expr_unexpected_operator'        => 'एक्स्प्रेशन गलती: अनपेक्षित $1 ओपरेटर',
	'pfunc_expr_missing_operand'            => 'एक्स्प्रेशन गलती: $1 का घटक मिला नहीं',
	'pfunc_expr_unexpected_closing_bracket' => 'एक्स्प्रेशन गलती: अनपेक्षित समाप्ति ब्रैकेट',
	'pfunc_expr_unrecognised_punctuation'   => 'एक्स्प्रेशन गलती: अनपेक्षित उद्गार चिन्ह "$1"',
	'pfunc_expr_unclosed_bracket'           => 'एक्स्प्रेशन गलती: ब्रैकेट बंद नहीं किया',
	'pfunc_expr_division_by_zero'           => 'शून्य से भाग',
	'pfunc_expr_invalid_argument'           => '$1: < -1 or > 1 के लिए अमान्य कथन',
	'pfunc_expr_invalid_argument_ln'        => 'ln: <= 0 के लिए अमान्य कथन',
	'pfunc_expr_unknown_error'              => 'एक्स्प्रेशन गलती: अज्ञात गलती ($1)',
	'pfunc_expr_not_a_number'               => '$1 में: रिज़ल्ट संख्यामें नहीं हैं',
);

/** Croatian (Hrvatski)
 * @author SpeedyGonsales
 * @author Dalibor Bosits
 * @author Siebrand
 * @author Dnik
 */
$messages['hr'] = array(
	'pfunc_desc'                            => 'Proširite parser logičkim funkcijama',
	'pfunc_time_error'                      => 'Greška: oblik vremena nije valjan',
	'pfunc_time_too_long'                   => 'Greška: prevelik broj #time (vremenskih) poziva',
	'pfunc_rel2abs_invalid_depth'           => 'Greška: Nevaljana dubina putanje: "$1" (pokušaj pristupanja čvoru iznad korijenskog)',
	'pfunc_expr_stack_exhausted'            => 'Greška u predlošku: prepunjen stog',
	'pfunc_expr_unexpected_number'          => 'Greška u predlošku: Neočekivan broj',
	'pfunc_expr_preg_match_failure'         => 'Greška u predlošku: Neočekivana preg_match greška',
	'pfunc_expr_unrecognised_word'          => 'Greška u predlošku: Nepoznata riječ "$1"',
	'pfunc_expr_unexpected_operator'        => 'Greška u predlošku: Neočekivani operator $1',
	'pfunc_expr_missing_operand'            => 'Greška u predlošku: Operator $1 nedostaje',
	'pfunc_expr_unexpected_closing_bracket' => 'Greška u predlošku: Neočekivana zatvorena zagrada',
	'pfunc_expr_unrecognised_punctuation'   => 'Greška u predlošku: Nepoznat interpunkcijski znak "$1"',
	'pfunc_expr_unclosed_bracket'           => 'Greška u predlošku: Nezatvorene zagrade',
	'pfunc_expr_division_by_zero'           => 'Dijeljenje s nulom',
	'pfunc_expr_invalid_argument'           => 'Nevaljani argumenti za $1: < -1 ili > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Nevaljani argument za ln: <= 0',
	'pfunc_expr_unknown_error'              => 'Greška u predlošku: Nepoznata greška ($1)',
	'pfunc_expr_not_a_number'               => 'U $1: rezultat nije broj',
);

/** Upper Sorbian (Hornjoserbsce)
 * @author Michawiki
 */
$messages['hsb'] = array(
	'pfunc_desc'                            => 'Parser wo logiske funkcije rozšěrić',
	'pfunc_time_error'                      => 'Zmylk: njepłaćiwe časowe podaće',
	'pfunc_time_too_long'                   => 'Zmylk: přewjele zawołanjow #time',
	'pfunc_rel2abs_invalid_depth'           => 'Zmylk: Njepłaćiwa hłubokosć w pućiku: "$1" (Pospyt, zo by na suk wyše hłowneho suka dohrabnyło)',
	'pfunc_expr_stack_exhausted'            => 'Wurazowy zmylk: Staplowy skład wučerpany',
	'pfunc_expr_unexpected_number'          => 'Wurazowy zmylk: Njewočakowana ličba',
	'pfunc_expr_preg_match_failure'         => 'Wurazowy zmylk: Njewočakowana zmylna funkcija "preg_match"',
	'pfunc_expr_unrecognised_word'          => 'Wurazowy zmylk: Njespóznate słowo "$1"',
	'pfunc_expr_unexpected_operator'        => 'Wurazowy zmylk: Njewočakowany operator $1',
	'pfunc_expr_missing_operand'            => 'Wurazowy zmylk: Falowacy operand za $1',
	'pfunc_expr_unexpected_closing_bracket' => 'Wurazowy zmylk: Njewočakowana kónčna róžkata spinka',
	'pfunc_expr_unrecognised_punctuation'   => 'Wurazowy zmylk: Njespóznate interpunkciske znamješko "$1"',
	'pfunc_expr_unclosed_bracket'           => 'Wurazowy zmylk: Njewotzamknjena róžkata spinka',
	'pfunc_expr_division_by_zero'           => 'Diwizija přez nulu',
	'pfunc_expr_unknown_error'              => 'Wurazowy zmylk: Njeznaty zmylk ($1)',
	'pfunc_expr_not_a_number'               => 'W $1: Wuslědk ličba njeje',
);

/** Hungarian (Magyar)
 * @author Dani
 */
$messages['hu'] = array(
	'pfunc_desc'                            => 'Az értelmező kiegészítése logikai funkciókkal',
	'pfunc_time_error'                      => 'Hiba: érvénytelen idő',
	'pfunc_time_too_long'                   => 'Hiba: a #time túl sokszor lett meghívva',
	'pfunc_rel2abs_invalid_depth'           => 'Hiba: nem megfelelő a mélység az elérési útban: „$1” (egy olyan csomópontot akartál elérni, amely a gyökércsomópont felett van)',
	'pfunc_expr_stack_exhausted'            => 'Hiba a kifejezésben: a verem kiürült',
	'pfunc_expr_unexpected_number'          => 'Hiba a kifejezésben: nem várt szám',
	'pfunc_expr_preg_match_failure'         => 'Hiba a kifejezésben: a preg_match váratlanul hibát jelzett',
	'pfunc_expr_unrecognised_word'          => 'Hiba a kifejezésben: ismeretlen „$1” szó',
	'pfunc_expr_unexpected_operator'        => 'Hiba a kifejezésben: nem várt $1 operátor',
	'pfunc_expr_missing_operand'            => 'Hiba a kifejezésben: $1 egyik operandusa hiányzik',
	'pfunc_expr_unexpected_closing_bracket' => 'Hiba a kifejezésben: nem várt zárójel',
	'pfunc_expr_unrecognised_punctuation'   => 'Hiba a kifejezésben: ismeretlen „$1” központozó karakter',
	'pfunc_expr_unclosed_bracket'           => 'Hiba a kifejezésben: lezáratlan zárójel',
	'pfunc_expr_division_by_zero'           => 'Nullával való osztás',
	'pfunc_expr_invalid_argument'           => '$1 érvénytelen paramétert kapott: < -1 vagy > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Az ln érvénytelen paramétert kapott: <= 0',
	'pfunc_expr_unknown_error'              => 'Hiba a kifejezésben: ismeretlen hiba ($1)',
	'pfunc_expr_not_a_number'               => '$1: az eredmény nem szám',
);

/** Interlingua (Interlingua)
 * @author McDutchie
 */
$messages['ia'] = array(
	'pfunc_desc'                            => 'Meliorar le analysator syntactic con functiones logic',
	'pfunc_time_error'                      => 'Error: tempore invalide',
	'pfunc_time_too_long'                   => 'Error: troppo de appellos a #time',
	'pfunc_rel2abs_invalid_depth'           => 'Error: Profunditate invalide in cammino: "$1" (essayava acceder a un nodo superior al radice)',
	'pfunc_expr_stack_exhausted'            => 'Error in expression: Pila exhaurite',
	'pfunc_expr_unexpected_number'          => 'Error in expression: Numero non expectate',
	'pfunc_expr_preg_match_failure'         => 'Error in expression: Fallimento non expectate in preg_match',
	'pfunc_expr_unrecognised_word'          => 'Error in expression: Parola "$1" non recognoscite',
	'pfunc_expr_unexpected_operator'        => 'Error in expression: Operator $1 non expectate',
	'pfunc_expr_missing_operand'            => 'Error in expression: Manca un operando pro $1',
	'pfunc_expr_unexpected_closing_bracket' => 'Error in expression: Accollada clause non expectate',
	'pfunc_expr_unrecognised_punctuation'   => 'Error in expression: Character de punctuation "$1" non recognoscite',
	'pfunc_expr_unclosed_bracket'           => 'Error in expression: Accollada non claudite',
	'pfunc_expr_division_by_zero'           => 'Division per zero',
	'pfunc_expr_invalid_argument'           => 'Argumento invalide pro $1: < -1 o > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Argumento invalide pro ln: ≤ 0',
	'pfunc_expr_unknown_error'              => 'Error de expression: Error incognite ($1)',
	'pfunc_expr_not_a_number'               => 'In $1: le resultato non es un numero',
);

/** Indonesian (Bahasa Indonesia)
 * @author IvanLanin
 * @author Rex
 * @author Meursault2004
 */
$messages['id'] = array(
	'pfunc_desc'                            => 'Mengembangkan parser dengan fungsi logis',
	'pfunc_time_error'                      => 'Kesalahan: waktu tidak valid',
	'pfunc_time_too_long'                   => 'Kesalahan: Pemanggilan #time terlalu banyak',
	'pfunc_rel2abs_invalid_depth'           => 'Kesalahan: Kedalaman path tidak valid: "$1" (mencoba mengakses simpul di atas simpul akar)',
	'pfunc_expr_stack_exhausted'            => 'Kesalahan ekspresi: Stack habis',
	'pfunc_expr_unexpected_number'          => 'Kesalahan ekspresi: Angka yang tak terduga',
	'pfunc_expr_preg_match_failure'         => 'Kesalahan ekspresi: Kegagalan preg_match tak terduga',
	'pfunc_expr_unrecognised_word'          => 'Kesalahan ekspresi: Kata "$1" tak dikenal',
	'pfunc_expr_unexpected_operator'        => 'Kesalahan ekspresi: Operator $1 tak terduga',
	'pfunc_expr_missing_operand'            => 'Kesalahan ekspresi: Operand tak ditemukan untuk $1',
	'pfunc_expr_unexpected_closing_bracket' => 'Kesalahan ekspresi: Kurung tutup tak terduga',
	'pfunc_expr_unrecognised_punctuation'   => 'Kesalahan ekspresi: Karakter tanda baca "$1" tak dikenali',
	'pfunc_expr_unclosed_bracket'           => 'Kesalahan ekspresi: Kurung tanpa tutup',
	'pfunc_expr_division_by_zero'           => 'Pembagian oleh nol',
	'pfunc_expr_invalid_argument'           => 'Argumen tidak berlaku untuk $1: < -1 or > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Argumen tidak berlaku untuk ln: <= 0',
	'pfunc_expr_unknown_error'              => 'Kesalahan ekspresi: Kesalahan tak dikenal ($1)',
	'pfunc_expr_not_a_number'               => 'Pada $1: hasilnya bukan angka',
);

/** Ido (Ido)
 * @author Malafaya
 */
$messages['io'] = array(
	'pfunc_expr_division_by_zero' => 'Divido per zero',
);

/** Italian (Italiano)
 * @author BrokenArrow
 * @author Darth Kule
 * @author Pietrodn
 */
$messages['it'] = array(
	'pfunc_desc'                            => 'Aggiunge al parser una serie di funzioni logiche',
	'pfunc_time_error'                      => 'Errore: orario non valido',
	'pfunc_time_too_long'                   => 'Errore: troppe chiamate a #time',
	'pfunc_rel2abs_invalid_depth'           => 'Errore: profondità non valida nel percorso "$1" (si è tentato di accedere a un nodo superiore alla radice)',
	'pfunc_expr_stack_exhausted'            => "Errore nell'espressione: stack esaurito",
	'pfunc_expr_unexpected_number'          => "Errore nell'espressione: numero inatteso",
	'pfunc_expr_preg_match_failure'         => "Errore nell'espressione: errore inatteso in preg_match",
	'pfunc_expr_unrecognised_word'          => 'Errore nell\'espressione: parola "$1" non riconosciuta',
	'pfunc_expr_unexpected_operator'        => "Errore nell'espressione: operatore $1 inatteso",
	'pfunc_expr_missing_operand'            => "Errore nell'espressione: operando mancante per $1",
	'pfunc_expr_unexpected_closing_bracket' => "Errore nell'espressione: parentesi chiusa inattesa",
	'pfunc_expr_unrecognised_punctuation'   => 'Errore nell\'espressione: carattere di punteggiatura "$1" non riconosciuto',
	'pfunc_expr_unclosed_bracket'           => "Errore nell'espressione: parentesi non chiusa",
	'pfunc_expr_division_by_zero'           => 'Divisione per zero',
	'pfunc_expr_invalid_argument'           => 'Argomento non valido per $1: < -1 o > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Argomento non valido per ln: <= 0',
	'pfunc_expr_unknown_error'              => "Errore nell'espressione: errore sconosciuto ($1)",
	'pfunc_expr_not_a_number'               => 'In $1: il risultato non è un numero',
);

/** Japanese (日本語)
 * @author JtFuruhata
 */
$messages['ja'] = array(
	'pfunc_desc'                            => '論理関数によるパーサー拡張',
	'pfunc_time_error'                      => 'エラー: 時刻が不正です',
	'pfunc_time_too_long'                   => 'エラー: #time 呼び出しが多すぎます',
	'pfunc_rel2abs_invalid_depth'           => 'エラー: パス "$1" の階層が不正です（ルート階層からのアクセスをお試しください）',
	'pfunc_expr_stack_exhausted'            => '構文エラー: スタックが空です',
	'pfunc_expr_unexpected_number'          => '構文エラー: 予期せぬ数字です',
	'pfunc_expr_preg_match_failure'         => '構文エラー: 予期せぬ形で preg_match に失敗しました',
	'pfunc_expr_unrecognised_word'          => '構文エラー: "$1" は認識できません',
	'pfunc_expr_unexpected_operator'        => '構文エラー: 予期せぬ演算子 $1 があります',
	'pfunc_expr_missing_operand'            => '構文エラー: $1 のオペランドがありません',
	'pfunc_expr_unexpected_closing_bracket' => '構文エラー: 予期せぬ閉じ括弧です',
	'pfunc_expr_unrecognised_punctuation'   => '構文エラー: 認識できない区切り文字 "$1" があります',
	'pfunc_expr_unclosed_bracket'           => '構文エラー: 括弧が閉じられていません',
	'pfunc_expr_division_by_zero'           => '0で除算しました',
	'pfunc_expr_unknown_error'              => '構文エラー: 予期せぬエラー（$1）',
	'pfunc_expr_not_a_number'               => '$1: 結果が数字ではありません',
);

/** Javanese (Basa Jawa)
 * @author Meursault2004
 */
$messages['jv'] = array(
	'pfunc_desc'                            => 'Kembangna parser mawa fungsi logis',
	'pfunc_time_error'                      => 'Kaluputan: wektu ora absah',
	'pfunc_time_too_long'                   => 'Kaluputan: Olèhé nyeluk #time kakèhan',
	'pfunc_rel2abs_invalid_depth'           => 'Kaluputan: Kajeroané path ora absah: "$1" (nyoba ngakses simpul sadhuwuring simpul oyot)',
	'pfunc_expr_stack_exhausted'            => 'Kaluputan èksprèsi: Stack entèk',
	'pfunc_expr_unexpected_number'          => 'Kaluputan èksprèsi: Angka ora kaduga',
	'pfunc_expr_preg_match_failure'         => 'Kaluputan èksprèsi: Kaluputan preg_match sing ora kaduga',
	'pfunc_expr_unrecognised_word'          => 'Kaluputan èksprèsi: Tembung "$1" ora ditepungi',
	'pfunc_expr_unexpected_operator'        => 'Kaluputan èksprèsi: Operator $1 ora kaduga',
	'pfunc_expr_missing_operand'            => 'Kaluputan èksprèsi: Operand ora ditemokaké kanggo $1',
	'pfunc_expr_unexpected_closing_bracket' => 'Kaluputan èksprèsi: Kurung tutup ora kaduga',
	'pfunc_expr_unrecognised_punctuation'   => 'Kaluputan èksprèsi: Karakter tandha wacan "$1" ora ditepungi',
	'pfunc_expr_unclosed_bracket'           => 'Kaluputan èksprèsi: Kurung tanpa tutup',
	'pfunc_expr_division_by_zero'           => 'Dipara karo das (nol)',
	'pfunc_expr_invalid_argument'           => 'Argumèn ora absah kanggo $1: < -1 utawa > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Argumèn ora absah kanggo ln: <= 0',
	'pfunc_expr_unknown_error'              => 'Kaluputan èksprèsi: Kaluputan ora ditepungi ($1)',
	'pfunc_expr_not_a_number'               => 'Ing $1: pituwasé dudu angka',
);

/** Kazakh (Arabic script) (‫قازاقشا (تٴوتە)‬) */
$messages['kk-arab'] = array(
	'pfunc_time_error'                      => 'قاتە: جارامسىز ۋاقىت',
	'pfunc_time_too_long'                   => 'قاتە: #time شاقىرۋى تىم كوپ',
	'pfunc_rel2abs_invalid_depth'           => 'قاتە: مىنا جولدىڭ جارامسىز تەرەندىگى «$1» (تامىر ٴتۇيىننىڭ ۇستىندەگى تۇيىنگە قاتىناۋ تالابى)',
	'pfunc_expr_stack_exhausted'            => 'ايتىلىم قاتەسى: ستەك سارقىلدى',
	'pfunc_expr_unexpected_number'          => 'ايتىلىم قاتەسى: كۇتىلمەگەن سان',
	'pfunc_expr_preg_match_failure'         => 'ايتىلىم قاتەسى: كۇتىلمەگەن preg_match ساتسىزدىگى',
	'pfunc_expr_unrecognised_word'          => 'ايتىلىم قاتەسى: تانىلماعان ٴسوز «$1»',
	'pfunc_expr_unexpected_operator'        => 'ايتىلىم قاتەسى: كۇتىلمەگەن وپەراتور $1',
	'pfunc_expr_missing_operand'            => 'ايتىلىم قاتەسى: $1 ٴۇشىن جوعالعان وپەراند ',
	'pfunc_expr_unexpected_closing_bracket' => 'ايتىلىم قاتەسى: كۇتىلمەگەن جاباتىن جاقشا',
	'pfunc_expr_unrecognised_punctuation'   => 'ايتىلىم قاتەسى: تانىلماعان تىنىس بەلگىسى «$1» ',
	'pfunc_expr_unclosed_bracket'           => 'ايتىلىم قاتەسى: جابىلماعان جاقشا',
	'pfunc_expr_division_by_zero'           => 'نولگە ٴبولىنۋى',
	'pfunc_expr_unknown_error'              => 'ايتىلىم قاتەسى: بەلگىسىز قاتە ($1)',
	'pfunc_expr_not_a_number'               => '$1 دەگەندە: ناتىيجە سان ەمەس',
);

/** Kazakh (Cyrillic) (Қазақша (Cyrillic)) */
$messages['kk-cyrl'] = array(
	'pfunc_time_error'                      => 'Қате: жарамсыз уақыт',
	'pfunc_time_too_long'                   => 'Қате: #time шақыруы тым көп',
	'pfunc_rel2abs_invalid_depth'           => 'Қате: Мына жолдың жарамсыз терендігі «$1» (тамыр түйіннің үстіндегі түйінге қатынау талабы)',
	'pfunc_expr_stack_exhausted'            => 'Айтылым қатесі: Стек сарқылды',
	'pfunc_expr_unexpected_number'          => 'Айтылым қатесі: Күтілмеген сан',
	'pfunc_expr_preg_match_failure'         => 'Айтылым қатесі: Күтілмеген preg_match сәтсіздігі',
	'pfunc_expr_unrecognised_word'          => 'Айтылым қатесі: Танылмаған сөз «$1»',
	'pfunc_expr_unexpected_operator'        => 'Айтылым қатесі: Күтілмеген оператор $1',
	'pfunc_expr_missing_operand'            => 'Айтылым қатесі: $1 үшін жоғалған операнд ',
	'pfunc_expr_unexpected_closing_bracket' => 'Айтылым қатесі: Күтілмеген жабатын жақша',
	'pfunc_expr_unrecognised_punctuation'   => 'Айтылым қатесі: Танылмаған тыныс белгісі «$1» ',
	'pfunc_expr_unclosed_bracket'           => 'Айтылым қатесі: Жабылмаған жақша',
	'pfunc_expr_division_by_zero'           => 'Нөлге бөлінуі',
	'pfunc_expr_unknown_error'              => 'Айтылым қатесі: Белгісіз қате ($1)',
	'pfunc_expr_not_a_number'               => '$1 дегенде: нәтиже сан емес',
);

/** Kazakh (Latin) (Қазақша (Latin)) */
$messages['kk-latn'] = array(
	'pfunc_time_error'                      => 'Qate: jaramsız waqıt',
	'pfunc_time_too_long'                   => 'Qate: #time şaqırwı tım köp',
	'pfunc_rel2abs_invalid_depth'           => 'Qate: Mına joldıñ jaramsız terendigi «$1» (tamır tüýinniñ üstindegi tüýinge qatınaw talabı)',
	'pfunc_expr_stack_exhausted'            => 'Aýtılım qatesi: Stek sarqıldı',
	'pfunc_expr_unexpected_number'          => 'Aýtılım qatesi: Kütilmegen san',
	'pfunc_expr_preg_match_failure'         => 'Aýtılım qatesi: Kütilmegen preg_match sätsizdigi',
	'pfunc_expr_unrecognised_word'          => 'Aýtılım qatesi: Tanılmağan söz «$1»',
	'pfunc_expr_unexpected_operator'        => 'Aýtılım qatesi: Kütilmegen operator $1',
	'pfunc_expr_missing_operand'            => 'Aýtılım qatesi: $1 üşin joğalğan operand ',
	'pfunc_expr_unexpected_closing_bracket' => 'Aýtılım qatesi: Kütilmegen jabatın jaqşa',
	'pfunc_expr_unrecognised_punctuation'   => 'Aýtılım qatesi: Tanılmağan tınıs belgisi «$1» ',
	'pfunc_expr_unclosed_bracket'           => 'Aýtılım qatesi: Jabılmağan jaqşa',
	'pfunc_expr_division_by_zero'           => 'Nölge bölinwi',
	'pfunc_expr_unknown_error'              => 'Aýtılım qatesi: Belgisiz qate ($1)',
	'pfunc_expr_not_a_number'               => '$1 degende: nätïje san emes',
);

/** Khmer (ភាសាខ្មែរ)
 * @author Lovekhmer
 * @author គីមស៊្រុន
 */
$messages['km'] = array(
	'pfunc_time_error'            => 'កំហុស៖ ពេលវេលាមិនត្រឹមត្រូវ',
	'pfunc_expr_division_by_zero' => 'ចែកនឹងសូន្យ',
);

/** Korean (한국어)
 * @author ToePeu
 */
$messages['ko'] = array(
	'pfunc_time_error'            => '오류: 시간이 잘못되었습니다.',
	'pfunc_time_too_long'         => '오류: #time을 너무 많이 썼습니다.',
	'pfunc_expr_missing_operand'  => '표현 오류: $1의 피연산자가 없습니다.',
	'pfunc_expr_unclosed_bracket' => '표현 오류: 괄호를 닫지 않았습니다.',
	'pfunc_expr_division_by_zero' => '0으로 나눔',
	'pfunc_expr_unknown_error'    => '표현 오류: 알려지지 않은 오류 ($1)',
);

/** Luxembourgish (Lëtzebuergesch)
 * @author Robby
 */
$messages['lb'] = array(
	'pfunc_desc'                     => 'Erweidert Parser mat logesche Fonctiounen',
	'pfunc_time_error'               => 'Feeler: ongëlteg Zäit',
	'pfunc_expr_unexpected_number'   => 'Expressiouns-Feeler: Onerwarten Zuel',
	'pfunc_expr_unrecognised_word'   => 'Expressiouns-Feeler: Onerkantent Wuert "$1"',
	'pfunc_expr_unclosed_bracket'    => 'Expressiouns-Feeler: Eckeg Klammer net zougemaach',
	'pfunc_expr_division_by_zero'    => 'Divisioun duerch Null',
	'pfunc_expr_invalid_argument_ln' => 'Ongëltege Wert fir ln: <= 0',
	'pfunc_expr_unknown_error'       => 'Expression-Feeler: Onbekannte Feeler ($1)',
	'pfunc_expr_not_a_number'        => "An $1: D'Resultat ass keng Zuel",
);

/** Limburgish (Limburgs)
 * @author Matthias
 * @author Ooswesthoesbes
 */
$messages['li'] = array(
	'pfunc_desc'                            => 'Verrijkt de parser met logische functies',
	'pfunc_time_error'                      => 'Fout: ongeldige tied',
	'pfunc_time_too_long'                   => 'Fout: #time te vaok aangerope',
	'pfunc_rel2abs_invalid_depth'           => 'Fout: ongeldige diepte in pad: "$1" (probeerde \'n node bove de stamnode aan te rope)',
	'pfunc_expr_stack_exhausted'            => 'Fout in oetdrukking: stack oetgeput',
	'pfunc_expr_unexpected_number'          => 'Fout in oetdrukking: onverwacht getal',
	'pfunc_expr_preg_match_failure'         => 'Fout in oetdrukking: onverwacht fale van preg_match',
	'pfunc_expr_unrecognised_word'          => 'Fout in oetdrukking: woord "$1" neet herkend',
	'pfunc_expr_unexpected_operator'        => 'Fout in oetdrukking: neet verwachte operator $1',
	'pfunc_expr_missing_operand'            => 'Fout in oetdrukking: operand veur $1 mist',
	'pfunc_expr_unexpected_closing_bracket' => 'Fout in oetdrukking: haakje sloete op onverwachte plaats',
	'pfunc_expr_unrecognised_punctuation'   => 'Fout in oetdrukking: neet herkend leesteke "$1"',
	'pfunc_expr_unclosed_bracket'           => 'Fout in oetdrukking: neet geslote haakje opene',
	'pfunc_expr_division_by_zero'           => 'Deiling door nul',
	'pfunc_expr_invalid_argument'           => 'Ongeldige paramaeter veur $1: < -1 of > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Ongeldige paramaeter veur ln: <= 0',
	'pfunc_expr_unknown_error'              => 'Fout in oetdrukking: ónbekindje fout ($1)',
	'pfunc_expr_not_a_number'               => 'In $1: rezultaot is gein getal',
);

/** Malayalam (മലയാളം)
 * @author Shijualex
 */
$messages['ml'] = array(
	'pfunc_desc'                   => 'ലോഗിക്കല്‍ ഫങ്ഷന്‍സ് ഉപയോഗിച്ച് പാര്‍സര്‍  എന്‍‌ഹാന്‍സ് ചെയ്യുക',
	'pfunc_time_error'             => 'പിഴവ്:അസാധുവായ സമയം',
	'pfunc_time_too_long'          => 'പിഴവ്: വളരെയധികം #സമയ കാളുകള്‍',
	'pfunc_expr_unexpected_number' => 'Expression error: പ്രതീക്ഷിക്കാത്ത സംഖ്യ',
	'pfunc_expr_unrecognised_word' => 'Expression error: "$1" എന്ന തിരിച്ചറിയാന്‍ സാധിക്കാഞ്ഞ വാക്ക്',
	'pfunc_expr_division_by_zero'  => 'പൂജ്യം കൊണ്ടുള്ള ഹരണം',
	'pfunc_expr_unknown_error'     => 'Expression error: കാരണം അജ്ഞാതമായ പിഴവ് ($1)',
	'pfunc_expr_not_a_number'      => '$1ല്‍: ഫലം ഒരു സംഖ്യയല്ല',
);

/** Marathi (मराठी)
 * @author Kaustubh
 */
$messages['mr'] = array(
	'pfunc_desc'                            => 'तार्किक कार्ये वापरून पार्सर वाढवा',
	'pfunc_time_error'                      => 'त्रुटी: चुकीचा वेळ',
	'pfunc_time_too_long'                   => 'त्रुटी: खूप जास्त #time कॉल्स',
	'pfunc_rel2abs_invalid_depth'           => 'त्रुटी: मार्गामध्ये चुकीची गहनता: "$1" (रूट नोडच्या वरील नोड शोधायचा प्रयत्न केला)',
	'pfunc_expr_stack_exhausted'            => 'एक्स्प्रेशन त्रुटी: स्टॅक संपला',
	'pfunc_expr_unexpected_number'          => 'एक्स्प्रेशन त्रुटी: अनपेक्षित क्रमांक',
	'pfunc_expr_preg_match_failure'         => 'एक्स्प्रेशन त्रुटी: अनपेक्षित preg_match रद्दीकरण',
	'pfunc_expr_unrecognised_word'          => 'एक्स्प्रेशन त्रुटी: अनोळखी शब्द "$1"',
	'pfunc_expr_unexpected_operator'        => 'एक्स्प्रेशन त्रुटी: अनोळखी $1 कार्यवाहक',
	'pfunc_expr_missing_operand'            => 'एक्स्प्रेशन त्रुटी: $1 चा घटक सापडला नाही',
	'pfunc_expr_unexpected_closing_bracket' => 'एक्स्प्रेशन त्रुटी: अनपेक्षित समाप्ती कंस',
	'pfunc_expr_unrecognised_punctuation'   => 'एक्स्प्रेशन त्रुटी: अनोळखी उद्गारवाचक चिन्ह "$1"',
	'pfunc_expr_unclosed_bracket'           => 'एक्स्प्रेशन त्रुटी: कंस समाप्त केलेला नाही',
	'pfunc_expr_division_by_zero'           => 'शून्य ने भागाकार',
	'pfunc_expr_invalid_argument'           => '$1 साठी अवैध अर्ग्युमेंट: < -1 किंवा > 1',
	'pfunc_expr_invalid_argument_ln'        => 'ln करिता अवैध अर्ग्युमेंट: <= 0',
	'pfunc_expr_unknown_error'              => 'एक्स्प्रेशन त्रुटी: अनोळखी त्रुटी ($1)',
	'pfunc_expr_not_a_number'               => '$1 मध्ये: निकाल संख्येत नाही',
);

/** Malay (Bahasa Melayu)
 * @author Aviator
 */
$messages['ms'] = array(
	'pfunc_desc'                            => 'Meningkatkan penghurai dengan fungsi-fungsi logik',
	'pfunc_time_error'                      => 'Ralat: waktu tidak sah',
	'pfunc_time_too_long'                   => 'Ralat: terlalu banyak panggilan #time',
	'pfunc_rel2abs_invalid_depth'           => 'Ralat: Kedalaman tidak sah dalam laluan: "$1" (cubaan mencapai nod di atas nod induk)',
	'pfunc_expr_stack_exhausted'            => 'Ralat ungkapan: Tindanan tuntas',
	'pfunc_expr_unexpected_number'          => 'Ralat ungkapan: Nombor tidak dijangka',
	'pfunc_expr_preg_match_failure'         => 'Ralat ungkapan: Kegagalan preg_match tidak dijangka',
	'pfunc_expr_unrecognised_word'          => 'Ralat ungkapan: Perkataan "$1" tidak dikenali',
	'pfunc_expr_unexpected_operator'        => 'Ralat ungkapan: Pengendali $1 tidak dijangka',
	'pfunc_expr_missing_operand'            => 'Ralat ungkapan: Kendalian bagi $1 tiada',
	'pfunc_expr_unexpected_closing_bracket' => 'Ralat ungkapan: Tanda kurung penutup tidak dijangka',
	'pfunc_expr_unrecognised_punctuation'   => 'Ralat ungkapan: Aksara tanda baca "$1" tidak dikenali',
	'pfunc_expr_unclosed_bracket'           => 'Ralat ungkapan: Tanda kurung tidak ditutup',
	'pfunc_expr_division_by_zero'           => 'Pembahagian dengan sifar',
	'pfunc_expr_invalid_argument'           => 'Argumen bagi $1 tidak sah: < -1 atau > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Argumen bagi ln tidak sah: <= 0',
	'pfunc_expr_unknown_error'              => 'Ralat ungkapan: Ralat tidak diketahui ($1)',
	'pfunc_expr_not_a_number'               => 'Dalam $1: hasil bukan nombor',
);

/** Low German (Plattdüütsch)
 * @author Slomox
 */
$messages['nds'] = array(
	'pfunc_time_error'                      => 'Fehler: mit de Tiet stimmt wat nich',
	'pfunc_time_too_long'                   => 'Fehler: #time warrt to faken opropen',
	'pfunc_rel2abs_invalid_depth'           => 'Fehler: Mit den Padd „$1“ stimmt wat nich, liggt nich ünner den Wuddelorner',
	'pfunc_expr_stack_exhausted'            => 'Fehler in’n Utdruck: Stack överlopen',
	'pfunc_expr_unexpected_number'          => 'Fehler in’n Utdruck: Unverwacht Tall',
	'pfunc_expr_preg_match_failure'         => 'Fehler in’n Utdruck: Unverwacht Fehler bi „preg_match“',
	'pfunc_expr_unrecognised_word'          => 'Fehler in’n Utdruck: Woort „$1“ nich kennt',
	'pfunc_expr_unexpected_operator'        => 'Fehler in’n Utdruck: Unverwacht Operator $1',
	'pfunc_expr_missing_operand'            => 'Fehler in’n Utdruck: Operand för $1 fehlt',
	'pfunc_expr_unexpected_closing_bracket' => 'Fehler in’n Utdruck: Unverwacht Klammer to',
	'pfunc_expr_unrecognised_punctuation'   => 'Fehler in’n Utdruck: Satzteken „$1“ nich kennt',
	'pfunc_expr_unclosed_bracket'           => 'Fehler in’n Utdruck: Nich slatene Klammer',
	'pfunc_expr_division_by_zero'           => 'Delen dör Null',
	'pfunc_expr_invalid_argument'           => 'Ungüllig Argument för $1: < -1 oder > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Ungüllig Argument för ln: <= 0',
	'pfunc_expr_unknown_error'              => 'Fehler in’n Utdruck: Unbekannten Fehler ($1)',
	'pfunc_expr_not_a_number'               => 'In $1: wat rutkamen is, is kene Tall',
);

/** Nepali (नेपाली)
 * @author SPQRobin
 */
$messages['ne'] = array(
	'pfunc_time_error'            => 'त्रुटी: गलत/वा हुदैनहुने समय',
	'pfunc_time_too_long'         => 'त्रुटी: एकदम धेरै #time callहरु',
	'pfunc_rel2abs_invalid_depth' => 'त्रुटी: पाथमा (इनभ्यालिड)गलत गहिराइ(डेप्थ) भयो: "$1" (ले रुट नोड भन्दापनि माथिको नोडलाइ चलाउन(एकसेस) गर्न खोज्यो)',
);

/** Dutch (Nederlands)
 * @author SPQRobin
 * @author Siebrand
 */
$messages['nl'] = array(
	'pfunc_desc'                            => 'Verrijkt de parser met logische functies',
	'pfunc_time_error'                      => 'Fout: ongeldige tijd',
	'pfunc_time_too_long'                   => 'Fout: #time te vaak aangeroepen',
	'pfunc_rel2abs_invalid_depth'           => 'Fout: ongeldige diepte in pad: "$1" (probeerde een node boven de stamnode aan te roepen)',
	'pfunc_expr_stack_exhausted'            => 'Fout in uitdrukking: stack uitgeput',
	'pfunc_expr_unexpected_number'          => 'Fout in uitdrukking: onverwacht getal',
	'pfunc_expr_preg_match_failure'         => 'Fout in uitdrukking: onverwacht falen van preg_match',
	'pfunc_expr_unrecognised_word'          => 'Fout in uitdrukking: woord "$1" niet herkend',
	'pfunc_expr_unexpected_operator'        => 'Fout in uitdrukking: niet verwachte operator $1',
	'pfunc_expr_missing_operand'            => 'Fout in uitdrukking: operand voor $1 mist',
	'pfunc_expr_unexpected_closing_bracket' => 'Fout in uitdrukking: haakje sluiten op onverwachte plaats',
	'pfunc_expr_unrecognised_punctuation'   => 'Fout in uitdrukking: niet herkend leesteken "$1"',
	'pfunc_expr_unclosed_bracket'           => 'Fout in uitdrukking: niet gesloten haakje openen',
	'pfunc_expr_division_by_zero'           => 'Deling door nul',
	'pfunc_expr_invalid_argument'           => 'Ongeldige parameter voor $1: < -1 of > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Ongeldige parameter voor ln: <= 0',
	'pfunc_expr_unknown_error'              => 'Fout in uitdrukking: onbekende fout ($1)',
	'pfunc_expr_not_a_number'               => 'In $1: resultaat is geen getal',
);

/** Norwegian Nynorsk (‪Norsk (nynorsk)‬)
 * @author Eirik
 */
$messages['nn'] = array(
	'pfunc_desc'                            => 'Legg til logiske funksjonar i parseren.',
	'pfunc_time_error'                      => 'Feil: Ugyldig tid',
	'pfunc_time_too_long'                   => 'Feil: #time er kalla for mange gonger',
	'pfunc_rel2abs_invalid_depth'           => 'Feil: Ugyldig djupn i stien: «$1» (prøvde å nå ein node ovanfor rotnoden)',
	'pfunc_expr_stack_exhausted'            => 'Feil i uttrykket: Stacken er tømd',
	'pfunc_expr_unexpected_number'          => 'Feil i uttrykket: Uventa tal',
	'pfunc_expr_preg_match_failure'         => 'Feil i uttrykket: Uventa feil i preg_match',
	'pfunc_expr_unrecognised_word'          => 'Feil i uttrykket: Ukjent ord, «$1»',
	'pfunc_expr_unexpected_operator'        => 'Feil i uttrykket: Uventa operatør, $1',
	'pfunc_expr_missing_operand'            => 'Feil i uttrykket: Operand for $1 manglar',
	'pfunc_expr_unexpected_closing_bracket' => 'Feil i uttrykket: Uventa avsluttande parentes',
	'pfunc_expr_unrecognised_punctuation'   => 'Feil i uttrykket: Ukjent punktumsteikn, «$1»',
	'pfunc_expr_unclosed_bracket'           => 'Feil i uttrykket: Ein parentes er ikkje avslutta',
	'pfunc_expr_division_by_zero'           => 'Divisjon med null',
	'pfunc_expr_unknown_error'              => 'Feil i uttrykket: Ukjend feil ($1)',
	'pfunc_expr_not_a_number'               => 'Resultatet i $1 er ikkje eit tal',
);

/** Norwegian (bokmål)‬ (‪Norsk (bokmål)‬)
 * @author Jon Harald Søby
 */
$messages['no'] = array(
	'pfunc_desc'                            => 'Utvid parser med logiske funksjoner',
	'pfunc_time_error'                      => 'Feil: ugyldig tid',
	'pfunc_time_too_long'                   => 'Feil: #time brukt for mange ganger',
	'pfunc_rel2abs_invalid_depth'           => 'Feil: Ugyldig dybde i sti: «$1» (prøvde å få tilgang til en node over rotnoden)',
	'pfunc_expr_stack_exhausted'            => 'Uttrykksfeil: Stakk utbrukt',
	'pfunc_expr_unexpected_number'          => 'Uttrykksfeil: Uventet nummer',
	'pfunc_expr_preg_match_failure'         => 'Uttrykksfeil: Uventet preg_match-feil',
	'pfunc_expr_unrecognised_word'          => 'Uttrykksfeil: Ugjenkjennelig ord «$1»',
	'pfunc_expr_unexpected_operator'        => 'Uttrykksfeil: Uventet $1-operator',
	'pfunc_expr_missing_operand'            => 'Uttrykksfeil: Mangler operand for $1',
	'pfunc_expr_unexpected_closing_bracket' => 'Uttrykksfeil: Uventet lukkende parentes',
	'pfunc_expr_unrecognised_punctuation'   => 'Uttrykksfeil: Ugjenkjennelig tegn «$1»',
	'pfunc_expr_unclosed_bracket'           => 'Uttrykksfeil: Åpen parentes',
	'pfunc_expr_division_by_zero'           => 'Deling på null',
	'pfunc_expr_invalid_argument'           => 'Ugyldig argument for $1: < -1 eller > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Ugyldig argument for ln: <= 0',
	'pfunc_expr_unknown_error'              => 'Uttrykksfeil: Ukjent feil ($1)',
	'pfunc_expr_not_a_number'               => 'I $1: resultat er ikke et tall',
);

/** Occitan (Occitan)
 * @author Cedric31
 * @author Siebrand
 */
$messages['oc'] = array(
	'pfunc_desc'                            => 'Augmenta lo parser amb de foncions logicas',
	'pfunc_time_error'                      => 'Error: durada invalida',
	'pfunc_time_too_long'                   => 'Error: parser #time apelat tròp de còps',
	'pfunc_rel2abs_invalid_depth'           => 'Error: nivèl de repertòri invalid dins lo camin : "$1" (a ensajat d’accedir a un nivèl al-dessús del repertòri raiç)',
	'pfunc_expr_stack_exhausted'            => 'Expression erronèa : pila agotada',
	'pfunc_expr_unexpected_number'          => 'Expression erronèa : nombre pas esperat',
	'pfunc_expr_preg_match_failure'         => 'Expression erronèa : una expression pas compresa a pas capitat',
	'pfunc_expr_unrecognised_word'          => "Error d'expression : lo mot '''$1''' es pas reconegut",
	'pfunc_expr_unexpected_operator'        => "Error d'expression : l'operator '''$1''' es pas reconegut",
	'pfunc_expr_missing_operand'            => "Error d'expression : l'operanda '''$1''' es pas reconeguda",
	'pfunc_expr_unexpected_closing_bracket' => "Error d'expression : parentèsi tampanta pas prevista",
	'pfunc_expr_unrecognised_punctuation'   => "Error d'expression : caractèr de ponctuacion « $1 » pas reconegut",
	'pfunc_expr_unclosed_bracket'           => 'Error d’expression : parentèsi pas tampada',
	'pfunc_expr_division_by_zero'           => 'Division per zèro',
	'pfunc_expr_invalid_argument'           => 'Valor incorrècta per $1 : < -1 o > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Valor incorrècta per ln : ≤ 0',
	'pfunc_expr_unknown_error'              => "Error d'expression : error desconeguda ($1)",
	'pfunc_expr_not_a_number'               => 'Dins $1 : lo resultat es pas un nombre',
);

/** Polish (Polski)
 * @author Sp5uhe
 * @author Derbeth
 */
$messages['pl'] = array(
	'pfunc_desc'                            => 'Rozszerza analizator składni o funkcje logiczne',
	'pfunc_time_error'                      => 'Błąd: niepoprawny czas',
	'pfunc_time_too_long'                   => 'Błąd: zbyt wiele wywołań funkcji #time',
	'pfunc_rel2abs_invalid_depth'           => 'Błąd: Nieprawidłowa głębokość w ścieżce: „$1” (próba dostępu do węzła powyżej korzenia)',
	'pfunc_expr_stack_exhausted'            => 'Błąd w wyrażeniu: Stos wyczerpany',
	'pfunc_expr_unexpected_number'          => 'Błąd w wyrażeniu: Nieoczekiwana liczba',
	'pfunc_expr_preg_match_failure'         => 'Błąd w wyrażeniu: Nieoczekiwany błąd preg_match',
	'pfunc_expr_unrecognised_word'          => 'Błąd w wyrażeniu: Nierozpoznane słowo „$1”',
	'pfunc_expr_unexpected_operator'        => 'Błąd w wyrażeniu: Nieoczekiwany operator $1',
	'pfunc_expr_missing_operand'            => 'Błąd w wyrażeniu: Brak argumentu funkcji $1',
	'pfunc_expr_unexpected_closing_bracket' => 'Błąd w wyrażeniu: Nieoczekiwany nawias zamykający',
	'pfunc_expr_unrecognised_punctuation'   => 'Błąd w wyrażeniu: Nierozpoznany znak interpunkcyjny „$1”',
	'pfunc_expr_unclosed_bracket'           => 'Błąd w wyrażeniu: Niedomknięty nawias',
	'pfunc_expr_division_by_zero'           => 'Dzielenie przez zero',
	'pfunc_expr_invalid_argument'           => 'Nieprawidłowy argument funkcji $1 – mniejszy od -1 lub większy od 1',
	'pfunc_expr_invalid_argument_ln'        => 'Nieprawidłowy argument funkcji ln – mniejszy lub równy 0',
	'pfunc_expr_unknown_error'              => 'Błąd w wyrażeniu: Nieznany błąd ($1)',
	'pfunc_expr_not_a_number'               => 'W $1: wynik nie jest liczbą',
);

/** Piedmontese (Piemontèis)
 * @author Bèrto 'd Sèra
 * @author Siebrand
 */
$messages['pms'] = array(
	'pfunc_time_error'            => 'Eror: temp nen bon',
	'pfunc_time_too_long'         => 'Eror: #time a ven ciamà tròpe vire',
	'pfunc_rel2abs_invalid_depth' => 'Eror: profondità nen bon-a ant ël përcors: "$1" (a l\'é provasse a ciamé un grop dzora a la rèis)',
);

/** Pashto (پښتو)
 * @author Ahmed-Najib-Biabani-Ibrahimkhel
 */
$messages['ps'] = array(
	'pfunc_time_error' => 'ستونزه: ناسم وخت',
);

/** Portuguese (Português)
 * @author Malafaya
 */
$messages['pt'] = array(
	'pfunc_desc'                            => 'Melhora o analisador "parser" com funções lógicas',
	'pfunc_time_error'                      => 'Erro: tempo inválido',
	'pfunc_time_too_long'                   => 'Erro: demasiadas chamadas a #time',
	'pfunc_rel2abs_invalid_depth'           => 'Erro: Profundidade inválida no caminho: "$1" (foi tentado o acesso a um nó acima do nó raiz)',
	'pfunc_expr_stack_exhausted'            => 'Erro de expressão: Pilha esgotada',
	'pfunc_expr_unexpected_number'          => 'Erro de expressão: Número inesperado',
	'pfunc_expr_preg_match_failure'         => 'Erro de expressão: Falha em preg_match inesperada',
	'pfunc_expr_unrecognised_word'          => 'Erro de expressão: Palavra "$1" não reconhecida',
	'pfunc_expr_unexpected_operator'        => 'Erro de expressão: Operador $1 inesperado',
	'pfunc_expr_missing_operand'            => 'Erro de expressão: Falta operando para $1',
	'pfunc_expr_unexpected_closing_bracket' => 'Erro de expressão: Parêntese de fecho inesperado',
	'pfunc_expr_unrecognised_punctuation'   => 'Erro de expressão: Caracter de pontuação "$1" não reconhecido',
	'pfunc_expr_unclosed_bracket'           => 'Erro de expressão: Parêntese não fechado',
	'pfunc_expr_division_by_zero'           => 'Divisão por zero',
	'pfunc_expr_invalid_argument'           => 'Argumento inválido para $1: < -1 or > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Argumento inválido para ln: <= 0',
	'pfunc_expr_unknown_error'              => 'Erro de expressão: Erro desconhecido ($1)',
	'pfunc_expr_not_a_number'               => 'Em $1: resultado não é um número',
);

/** Romanian (Română)
 * @author KlaudiuMihaila
 */
$messages['ro'] = array(
	'pfunc_time_error'                      => 'Eroare: timp incorect',
	'pfunc_time_too_long'                   => 'Eroare: prea multe apeluri #time',
	'pfunc_rel2abs_invalid_depth'           => 'Eroare: adâncime incorectă în cale: "$1" (încercat accesarea unui nod deasupra nodului rădăcină)',
	'pfunc_expr_unexpected_number'          => 'Eroare de expresie: număr neaşteptat',
	'pfunc_expr_preg_match_failure'         => 'Eroare de expresie: eşuare preg_match neaşteptată',
	'pfunc_expr_unrecognised_word'          => 'Eroare de expresie: "$1" este cuvânt necunoscut',
	'pfunc_expr_unexpected_operator'        => 'Eroare de expresie: operator $1 neaşteptat',
	'pfunc_expr_missing_operand'            => 'Eroare de expresie: operand lipsă pentru $1',
	'pfunc_expr_unexpected_closing_bracket' => 'Eroare de expresie: paranteză închisă neaşteptată',
	'pfunc_expr_unrecognised_punctuation'   => 'Eroare de expresie: caracter de punctuaţie "$1" necunoscut',
	'pfunc_expr_unclosed_bracket'           => 'Eroare de expresie: paranteză neînchisă',
	'pfunc_expr_division_by_zero'           => 'Împărţire la zero',
	'pfunc_expr_invalid_argument'           => 'Argument incorect pentru $1: < -1 sau > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Argument incorect pentru ln: <= 0',
	'pfunc_expr_unknown_error'              => 'Eroare de expresie: eroare necunoscută ($1)',
	'pfunc_expr_not_a_number'               => 'În $1: rezultatul nu este un număr',
);

/** Russian (Русский)
 * @author Александр Сигачёв
 */
$messages['ru'] = array(
	'pfunc_desc'                            => 'Улучшенный синтаксический анализатор с логическими функциями',
	'pfunc_time_error'                      => 'Ошибка: неправильное время',
	'pfunc_time_too_long'                   => 'Ошибка: слишком много вызовов функции #time',
	'pfunc_rel2abs_invalid_depth'           => 'Ошибка: ошибочная глубина пути: «$1» (попытка доступа к узлу, находящемуся выше, чем корневой)',
	'pfunc_expr_stack_exhausted'            => 'Ошибка выражения: переполнение стека',
	'pfunc_expr_unexpected_number'          => 'Ошибка выражения: неожидаемое число',
	'pfunc_expr_preg_match_failure'         => 'Ошибка выражения: сбой preg_match',
	'pfunc_expr_unrecognised_word'          => 'Ошибка выражения: неопознанное слово «$1»',
	'pfunc_expr_unexpected_operator'        => 'Ошибка выражения: неожидаемый оператор $1',
	'pfunc_expr_missing_operand'            => 'Ошибка выражения: $1 не хватает операнда',
	'pfunc_expr_unexpected_closing_bracket' => 'Ошибка выражения: неожидаемая закрывающая скобка',
	'pfunc_expr_unrecognised_punctuation'   => 'Ошибка выражения: неопознанный символ пунктуации «$1»',
	'pfunc_expr_unclosed_bracket'           => 'Ошибка выражения: незакрытая скобка',
	'pfunc_expr_division_by_zero'           => 'Деление на ноль',
	'pfunc_expr_invalid_argument'           => 'Ошибочный аргумент $1: < -1 или > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Ошибочный аргумент ln: <= 0',
	'pfunc_expr_unknown_error'              => 'Ошибка выражения: неизвестная ошибка ($1)',
	'pfunc_expr_not_a_number'               => 'В $1: результат не является числом',
);

/** Yakut (Саха тыла)
 * @author HalanTul
 */
$messages['sah'] = array(
	'pfunc_desc'                            => 'Логическай функциялаах тупсарыллыбыт синтаксическай анализатор',
	'pfunc_time_error'                      => 'Алҕас: сыыһа кэм',
	'pfunc_time_too_long'                   => 'Алҕас: #time функция наһаа элбэхтик хатыламмыт',
	'pfunc_rel2abs_invalid_depth'           => 'Алҕас: ошибочная глубина пути: «$1» (попытка доступа к узлу, находящемуся выше, чем корневой)',
	'pfunc_expr_stack_exhausted'            => 'Ошибка выражения: переполнение стека',
	'pfunc_expr_unexpected_number'          => 'Алҕас: кэтэһиллибэтэх чыыһыла',
	'pfunc_expr_preg_match_failure'         => 'Алҕас: preg_match моһуоктанна',
	'pfunc_expr_unrecognised_word'          => 'Алҕас: биллибэт тыл «$1»',
	'pfunc_expr_unexpected_operator'        => 'Алҕас: кэтэһиллибэтэх оператор $1',
	'pfunc_expr_missing_operand'            => 'Алҕас: $1 операнда тиийбэт',
	'pfunc_expr_unexpected_closing_bracket' => 'Алҕас: кэтэһиллибэтэх сабар ускуопка',
	'pfunc_expr_unrecognised_punctuation'   => 'Алҕас: биллибэт пунктуация бэлиэтэ «$1»',
	'pfunc_expr_unclosed_bracket'           => 'Алҕас: сабыллыбатах ускуопка',
	'pfunc_expr_division_by_zero'           => 'Нуулга түҥэттии',
	'pfunc_expr_invalid_argument'           => '$1 алҕас аргуменнаах: < -1 or > 1',
	'pfunc_expr_invalid_argument_ln'        => 'ln аргумена сыыһалаах: <= 0',
	'pfunc_expr_unknown_error'              => 'Expression error (ошибка выражения): Биллибэт алҕас ($1)',
	'pfunc_expr_not_a_number'               => '$1 иһигэр: эппиэтэ чыыһыла буолбатах',
);

/** Slovak (Slovenčina)
 * @author Helix84
 */
$messages['sk'] = array(
	'pfunc_desc'                            => 'Rozšírenie syntaktického analyzátora o logické funkcie',
	'pfunc_time_error'                      => 'Chyba: Neplatný čas',
	'pfunc_time_too_long'                   => 'Chyba: príliš veľa volaní #time',
	'pfunc_rel2abs_invalid_depth'           => 'Chyba: Neplatná hĺbka v ceste: „$1“ (pokus o prístup k uzlu nad koreňovým uzlom)',
	'pfunc_expr_stack_exhausted'            => 'Chyba výrazu: Zásobník vyčerpaný',
	'pfunc_expr_unexpected_number'          => 'Chyba výrazu: Neočakávané číslo',
	'pfunc_expr_preg_match_failure'         => 'Chyba výrazu: Neočakávané zlyhanie funkcie preg_match',
	'pfunc_expr_unrecognised_word'          => 'Chyba výrazu: Nerozpoznané slovo „$1“',
	'pfunc_expr_unexpected_operator'        => 'Chyba výrazu: Neočakávaný operátor $1',
	'pfunc_expr_missing_operand'            => 'Chyba výrazu: Chýbajúci operand pre $1',
	'pfunc_expr_unexpected_closing_bracket' => 'Chyba výrazu: Neočakávaná zatvárajúca hranatá zátvorka',
	'pfunc_expr_unrecognised_punctuation'   => 'Chyba výrazu: Nerozpoznané diakritické znamienko „$1“',
	'pfunc_expr_unclosed_bracket'           => 'Chyba výrazu: Neuzavretá hranatá zátvorka',
	'pfunc_expr_division_by_zero'           => 'Chyba výrazu: Delenie nulou',
	'pfunc_expr_invalid_argument'           => 'Neplatný argument pre $1: < -1 alebo > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Neplatný argument pre ln: <= 0',
	'pfunc_expr_unknown_error'              => 'Chyba výrazu: Neznáma chyba ($1)',
	'pfunc_expr_not_a_number'               => 'V $1: výsledok nie je číslo',
);

/** Serbian Cyrillic ekavian (ћирилица)
 * @author Millosh
 */
$messages['sr-ec'] = array(
	'pfunc_desc'                            => 'обогати парсер логичким функцијама',
	'pfunc_time_error'                      => 'Грешка: лоше време',
	'pfunc_time_too_long'                   => 'Грешка: превише #time позива',
	'pfunc_expr_stack_exhausted'            => 'Грешка у изразу: стек напуњен',
	'pfunc_expr_unexpected_number'          => 'Грешка у изразу: неочекивани број',
	'pfunc_expr_preg_match_failure'         => 'Грешка у изразу: Неочекивана preg_match грешка',
	'pfunc_expr_unrecognised_word'          => 'Грешка у изразу: непозната реч "$1"',
	'pfunc_expr_unexpected_operator'        => 'Грешка у изразу: непознати оператор "$1"',
	'pfunc_expr_missing_operand'            => 'Грешка у изразу: недостаје операнд за $1',
	'pfunc_expr_unexpected_closing_bracket' => 'Грешка у изразу: Неочекивано затварање средње заграде.',
	'pfunc_expr_unrecognised_punctuation'   => 'Грешка у изразу: Непознати интерпункцијски карактер "$1".',
	'pfunc_expr_unclosed_bracket'           => 'Грешка у изразу: Незатворена средња заграда.',
	'pfunc_expr_division_by_zero'           => 'Дељење са нулом.',
	'pfunc_expr_invalid_argument'           => 'Лош аргумент: $1 је < -1 или > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Лош аргумент: ln <= 0',
	'pfunc_expr_unknown_error'              => 'Грешка у изразу: Непозната грешка ($1)',
	'pfunc_expr_not_a_number'               => 'Резултат у $1 није број.',
);

/** Seeltersk (Seeltersk)
 * @author Pyt
 */
$messages['stq'] = array(
	'pfunc_time_error'                      => 'Failer: uungultige Tiedangoawe',
	'pfunc_time_too_long'                   => 'Failer: tou fuul #time-Aproupe',
	'pfunc_rel2abs_invalid_depth'           => 'Failer: uungultige Djüpte in Paad: „$1“ (Fersäik, ap n Knättepunkt buppe dän Haudknättepunkt toutougriepen)',
	'pfunc_expr_stack_exhausted'            => 'Expression-Failer: Stack-Uurloop',
	'pfunc_expr_unexpected_number'          => 'Expression-Failer: Nit ferwachtede Taal',
	'pfunc_expr_preg_match_failure'         => 'Expression-Failer: Uunferwachtede „preg_match“-Failfunktion',
	'pfunc_expr_unrecognised_word'          => 'Expression-Failer: Nit wierkoand Woud „$1“',
	'pfunc_expr_unexpected_operator'        => 'Expression-Failer: Uunferwachteden Operator: <strong><tt>$1</tt></strong>',
	'pfunc_expr_missing_operand'            => 'Expression-Failer: Failenden Operand foar <strong><tt>$1</tt></strong>',
	'pfunc_expr_unexpected_closing_bracket' => 'Expression-Failer: Uunferwachte sluutende kaantige Klammere',
	'pfunc_expr_unrecognised_punctuation'   => 'Expression-Failer: Nit wierkoand Satsteeken „$1“',
	'pfunc_expr_unclosed_bracket'           => 'Expression-Failer: Nit sleetene kaantige Klammer',
	'pfunc_expr_division_by_zero'           => 'Expression-Failer: Division truch Null',
	'pfunc_expr_unknown_error'              => 'Expression-Failer: Uunbekoanden Failer ($1)',
	'pfunc_expr_not_a_number'               => 'Expression-Failer: In $1: Resultoat is neen Taal',
);

/** Sundanese (Basa Sunda)
 * @author Kandar
 * @author Irwangatot
 */
$messages['su'] = array(
	'pfunc_desc'       => 'Ngembangkeun parser kalawan fungsi logis',
	'pfunc_time_error' => 'Éror: titimangsa teu valid',
);

/** Swedish (Svenska)
 * @author Lejonel
 * @author M.M.S.
 */
$messages['sv'] = array(
	'pfunc_desc'                            => 'Lägger till logiska funktioner i parsern',
	'pfunc_time_error'                      => 'Fel: ogiltig tid',
	'pfunc_time_too_long'                   => 'Fel: för många anrop av #time',
	'pfunc_rel2abs_invalid_depth'           => 'Fel: felaktig djup i sökväg: "$1" (försöker nå en nod ovanför rotnoden)',
	'pfunc_expr_stack_exhausted'            => 'Fel i uttryck: Stackutrymmet tog slut',
	'pfunc_expr_unexpected_number'          => 'Fel i uttryck: Oväntat tal',
	'pfunc_expr_preg_match_failure'         => 'Fel i uttryck: Oväntad fel i preg_match',
	'pfunc_expr_unrecognised_word'          => 'Fel i uttryck: Okänt ord "$1"',
	'pfunc_expr_unexpected_operator'        => 'Fel i uttryck: Oväntad operator $1',
	'pfunc_expr_missing_operand'            => 'Fel i uttryck: Operand saknas för $1',
	'pfunc_expr_unexpected_closing_bracket' => 'Fel i uttryck: Oväntad avslutande parentes',
	'pfunc_expr_unrecognised_punctuation'   => 'Fel i uttryck: Okänt interpunktionstecken "$1"',
	'pfunc_expr_unclosed_bracket'           => 'Fel i uttryck: Oavslutad parentes',
	'pfunc_expr_division_by_zero'           => 'Division med noll',
	'pfunc_expr_invalid_argument'           => 'Ogiltigt argument för $1: < -1 eller > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Ogiltigt argument för ln: <= 0',
	'pfunc_expr_unknown_error'              => 'Fel i uttryck: Okänt fel ($1)',
	'pfunc_expr_not_a_number'               => 'I $1: resultatet är inte ett tal',
);

/** Telugu (తెలుగు)
 * @author Mpradeep
 * @author Veeven
 */
$messages['te'] = array(
	'pfunc_time_error'                      => 'లోపం: సమయం సరిగ్గా లేదు',
	'pfunc_time_too_long'                   => 'లోపం: #timeను చాలా సార్లు ఉపయోగించారు',
	'pfunc_rel2abs_invalid_depth'           => 'లోపం: పాత్ యొక్క డెప్తు సరిగ్గాలేదు: "$1" (రూట్ నోడు కంటే పైన ఉన్న నోడు ఉపయోగించటానికి ప్రయత్నం జరిగింది)',
	'pfunc_expr_stack_exhausted'            => 'సమాసంలో(Expression) లోపం: స్టాకు మొత్తం అయిపోయింది',
	'pfunc_expr_unexpected_number'          => 'సమాసంలో(Expression) లోపం: ఊహించని సంఖ్య వచ్చింది',
	'pfunc_expr_preg_match_failure'         => 'సమాసంలో(Expression) లోపం: preg_matchలో ఊహించని విఫలం',
	'pfunc_expr_unrecognised_word'          => 'సమాసంలో(Expression) లోపం: "$1" అనే పదాన్ని గుర్తుపట్టలేకపోతున్నాను',
	'pfunc_expr_unexpected_operator'        => 'సమాసంలో(Expression) లోపం: $1 పరికర్తను(operator) ఊహించలేదు',
	'pfunc_expr_missing_operand'            => 'సమాసంలో(Expression) లోపం: $1కు ఒక ఆపరాండును ఇవ్వలేదు',
	'pfunc_expr_unexpected_closing_bracket' => 'సమాసంలో(Expression) లోపం: ఊహించని బ్రాకెట్టు ముగింపు',
	'pfunc_expr_unrecognised_punctuation'   => 'సమాసంలో(Expression) లోపం: "$1" అనే విరామ చిహ్నాన్ని గుర్తించలేకపోతున్నాను',
	'pfunc_expr_unclosed_bracket'           => 'సమాసంలో(Expression) లోపం: బ్రాకెట్టును మూయలేదు',
	'pfunc_expr_division_by_zero'           => 'సున్నాతో భాగించారు',
	'pfunc_expr_unknown_error'              => 'సమాసంలో(Expression) లోపం: తెలియని లోపం ($1)',
	'pfunc_expr_not_a_number'               => '$1లో: వచ్చిన ఫలితం సంఖ్య కాదు',
);

/** Tajik (Cyrillic) (Тоҷикӣ/tojikī (Cyrillic))
 * @author Ibrahim
 */
$messages['tg-cyrl'] = array(
	'pfunc_desc'                            => 'Ба таҷзеҳкунанда, дастурҳои мантиқӣ меафзояд',
	'pfunc_time_error'                      => 'Хато: замони ғайримиҷоз',
	'pfunc_time_too_long'                   => 'Хато: #time фарохонии беш аз ҳад',
	'pfunc_rel2abs_invalid_depth'           => 'Хато: Чуқурии ғайримиҷоз дар нишонӣ: "$1" (талош барои дастраси ба як нишонӣ болотар аз нишонии реша)',
	'pfunc_expr_stack_exhausted'            => 'Хатои ибора: Пушта аз даст рафтааст',
	'pfunc_expr_unexpected_number'          => 'Хатои ибора: Адади ғайримунтазир',
	'pfunc_expr_preg_match_failure'         => 'Хатои ибора: Хатои ғайримунтазири preg_match',
	'pfunc_expr_unrecognised_word'          => 'Хатои ибора: Калимаи ношинохта "$1"',
	'pfunc_expr_unexpected_operator'        => 'Хатои ибора: Амалгари ғайримунтазири $1',
	'pfunc_expr_missing_operand'            => 'Хатои ибора: Амалгари гумшуда барои  $1',
	'pfunc_expr_unexpected_closing_bracket' => 'Хатои ибора: Қафси бастаи номунтазир',
	'pfunc_expr_unrecognised_punctuation'   => 'Хатои ибора: Аломати нуқтагузории шинохтанашуда "$1"',
	'pfunc_expr_unclosed_bracket'           => 'Хатои ибора: Қафси бастанашуда',
	'pfunc_expr_division_by_zero'           => 'Тақсим бар сифр',
	'pfunc_expr_unknown_error'              => 'Хатои ибора: Хатои ношинос ($1)',
	'pfunc_expr_not_a_number'               => 'Дар $1: натиҷа адад нест',
);

/** Ukrainian (Українська)
 * @author Ahonc
 */
$messages['uk'] = array(
	'pfunc_desc'                            => 'Покращений синтаксичний аналізатор з логічними функціями',
	'pfunc_time_error'                      => 'Помилка: неправильний час',
	'pfunc_time_too_long'                   => 'Помилка: забагато викликів функції #time',
	'pfunc_rel2abs_invalid_depth'           => 'Помилка: неправильна глибина шляху: «$1» (спроба доступу до вузла, що знаходиться вище, ніж кореневий)',
	'pfunc_expr_stack_exhausted'            => 'Помилка виразу: стек переповнений',
	'pfunc_expr_unexpected_number'          => 'Помилка виразу: неочікуване число',
	'pfunc_expr_preg_match_failure'         => 'Помилка виразу: збій preg_match',
	'pfunc_expr_unrecognised_word'          => 'Помилка виразу: незрозуміле слово «$1»',
	'pfunc_expr_unexpected_operator'        => 'Помилка виразу: неочікуваний оператор $1',
	'pfunc_expr_missing_operand'            => 'Помилка виразу: бракує операнда для $1',
	'pfunc_expr_unexpected_closing_bracket' => 'Помилка виразу: неочікувана закрита дужка',
	'pfunc_expr_unrecognised_punctuation'   => 'Помилка виразу: незрозумілий розділовий знак «$1»',
	'pfunc_expr_unclosed_bracket'           => 'Помилка виразу: незакрита дужка',
	'pfunc_expr_division_by_zero'           => 'Ділення на нуль',
	'pfunc_expr_invalid_argument'           => 'Неправильний аргумент для $1: < -1 або > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Помилковий аргумент логарифма (має бути більший від нуля)',
	'pfunc_expr_unknown_error'              => 'Помилка виразу: невідома помилка ($1)',
	'pfunc_expr_not_a_number'               => 'У $1: результат не є числом',
);

/** Vèneto (Vèneto)
 * @author Candalua
 */
$messages['vec'] = array(
	'pfunc_desc'                            => 'Zonta al parser na serie de funsion logiche',
	'pfunc_time_error'                      => 'Eror: orario mìa valido',
	'pfunc_time_too_long'                   => 'Eror: massa chiamate a #time',
	'pfunc_rel2abs_invalid_depth'           => 'Eror: profondità mìa valida nel percorso "$1" (se gà proà a accédar a un nodo piassè sora de la raìsa)',
	'pfunc_expr_stack_exhausted'            => "Eror ne l'espression: stack esaurìo",
	'pfunc_expr_unexpected_number'          => "Eror ne l'espression: xe vegnù fora un nùmaro che no se se spetava",
	'pfunc_expr_preg_match_failure'         => "Eror ne l'espression: eror inateso in preg_match",
	'pfunc_expr_unrecognised_word'          => 'Eror ne l\'espression: parola "$1" mìa riconossiùa',
	'pfunc_expr_unexpected_operator'        => "Eror ne l'espression: operator $1 inateso",
	'pfunc_expr_missing_operand'            => "Eror ne l'espression: operando mancante par $1",
	'pfunc_expr_unexpected_closing_bracket' => "Eror ne l'espression: parentesi chiusa inatesa",
	'pfunc_expr_unrecognised_punctuation'   => 'Eror ne l\'espression: caràtere de puntegiatura "$1" mìa riconossiùo',
	'pfunc_expr_unclosed_bracket'           => "Eror ne l'espression: parentesi verta e mìa sarà",
	'pfunc_expr_division_by_zero'           => 'Division par zero',
	'pfunc_expr_invalid_argument'           => 'Argomento mìa valido par $1: < -1 or > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Argomento mìa valido par ln: <= 0',
	'pfunc_expr_unknown_error'              => "Eror ne l'espression: eror sconossiùo ($1)",
	'pfunc_expr_not_a_number'               => "In $1: el risultato no'l xe mìa un nùmaro",
);

/** Vietnamese (Tiếng Việt)
 * @author Vinhtantran
 * @author Minh Nguyen
 */
$messages['vi'] = array(
	'pfunc_desc'                            => 'Nâng cao bộ xử lý với những hàm cú pháp lôgic',
	'pfunc_time_error'                      => 'Lỗi: thời gian không hợp lệ',
	'pfunc_time_too_long'                   => 'Lỗi: quá nhiều lần gọi #time',
	'pfunc_rel2abs_invalid_depth'           => 'Lỗi: độ sâu không hợp lệ trong đường dẫn “$1” (do cố gắng truy cập nút phía trên nút gốc)',
	'pfunc_expr_stack_exhausted'            => 'Lỗi biểu thức: Đã cạn stack',
	'pfunc_expr_unexpected_number'          => 'Lỗi biểu thức: Dư số',
	'pfunc_expr_preg_match_failure'         => 'Lỗi biểu thức: Hàm preg_match thất bại',
	'pfunc_expr_unrecognised_word'          => 'Lỗi biểu thức: Từ “$1” không rõ ràng',
	'pfunc_expr_unexpected_operator'        => "Lỗi biểu thức: Dư toán tử '''$1'''",
	'pfunc_expr_missing_operand'            => 'Lỗi biểu thức: Thiếu toán hạng trong $1',
	'pfunc_expr_unexpected_closing_bracket' => 'Lỗi biểu thức: Dư dấu đóng ngoặc',
	'pfunc_expr_unrecognised_punctuation'   => 'Lỗi biểu thức: Dấu câu “$1” không rõ ràng',
	'pfunc_expr_unclosed_bracket'           => 'Lỗi biểu thức: Dấu ngoặc chưa được đóng',
	'pfunc_expr_division_by_zero'           => 'Chia cho zero',
	'pfunc_expr_invalid_argument'           => 'Tham số không hợp lệ cho $1: < −1 hay > 1',
	'pfunc_expr_invalid_argument_ln'        => 'Tham số không hợp lệ cho ln: ≤ 0',
	'pfunc_expr_unknown_error'              => 'Lỗi biểu thức: Lỗi không rõ nguyên nhân ($1)',
	'pfunc_expr_not_a_number'               => 'Trong $1: kết quả không phải là kiểu số',
);

/** Volapük (Volapük)
 * @author Smeira
 */
$messages['vo'] = array(
	'pfunc_time_error'            => 'Pök: tim no lonöföl',
	'pfunc_expr_division_by_zero' => 'Müedam dub ser',
	'pfunc_expr_not_a_number'     => 'In $1: sek no binon num',
);

/** Yue (粵語)
 * @author Shinjiman
 */
$messages['yue'] = array(
	'pfunc_desc'                            => '用邏輯功能去加強處理器',
	'pfunc_time_error'                      => '錯: 唔啱嘅時間',
	'pfunc_time_too_long'                   => '錯: 太多 #time 呼叫',
	'pfunc_rel2abs_invalid_depth'           => '錯: 唔啱路徑嘅深度: "$1" (已經試過由頭點落個點度)',
	'pfunc_expr_stack_exhausted'            => '表達錯: 堆叠耗盡',
	'pfunc_expr_unexpected_number'          => '表達錯: 未預料嘅數字',
	'pfunc_expr_preg_match_failure'         => '表達錯: 未預料嘅 preg_match失敗',
	'pfunc_expr_unrecognised_word'          => '表達錯: 未預料嘅字 "$1"',
	'pfunc_expr_unexpected_operator'        => '表達錯: 未預料嘅 $1 運算符',
	'pfunc_expr_missing_operand'            => '表達錯: 缺少 $1 嘅運算符',
	'pfunc_expr_unexpected_closing_bracket' => '表達錯: 未預料嘅閂括號',
	'pfunc_expr_unrecognised_punctuation'   => '表達錯: 未能認得到嘅標點 "$1"',
	'pfunc_expr_unclosed_bracket'           => '表達錯: 未閂好嘅括號',
	'pfunc_expr_division_by_zero'           => '除以零',
	'pfunc_expr_invalid_argument'           => '$1嘅無效參數: < -1 or > 1',
	'pfunc_expr_invalid_argument_ln'        => 'ln嘅無效參數: <= 0',
	'pfunc_expr_unknown_error'              => '表達錯: 未知嘅錯 ($1)',
	'pfunc_expr_not_a_number'               => '響 $1: 結果唔係數字',
);

/** Simplified Chinese (‪中文(简体)‬)
 * @author Shinjiman
 */
$messages['zh-hans'] = array(
	'pfunc_desc'                            => '用逻辑功能去加强处理器',
	'pfunc_time_error'                      => '错误: 不正确的时间',
	'pfunc_time_too_long'                   => '错误: 过多 #time 的呼叫',
	'pfunc_rel2abs_invalid_depth'           => '错误: 不正确的路径深度: "$1" (已经尝试在顶点访问该点)',
	'pfunc_expr_stack_exhausted'            => '表达错误: 堆叠耗尽',
	'pfunc_expr_unexpected_number'          => '表达错误: 未预料的数字',
	'pfunc_expr_preg_match_failure'         => '表达错误: 未预料的 preg_match失败',
	'pfunc_expr_unrecognised_word'          => '表达错误: 未预料的字 "$1"',
	'pfunc_expr_unexpected_operator'        => '表达错误: 未预料的 $1 运算符',
	'pfunc_expr_missing_operand'            => '表达错误: 缺少 $1 的运算符',
	'pfunc_expr_unexpected_closing_bracket' => '表达错误: 未预料的关括号',
	'pfunc_expr_unrecognised_punctuation'   => '表达错误: 未能认得到的标点 "$1"',
	'pfunc_expr_unclosed_bracket'           => '表达错误: 未关闭的括号',
	'pfunc_expr_division_by_zero'           => '除以零',
	'pfunc_expr_invalid_argument'           => '$1的无效参数: < -1 or > 1',
	'pfunc_expr_invalid_argument_ln'        => 'ln的无效参数: <= 0',
	'pfunc_expr_unknown_error'              => '表达错误: 未知?错 ($1)',
	'pfunc_expr_not_a_number'               => '于 $1: 结果不是数字',
);

/** Traditional Chinese (‪中文(繁體)‬)
 * @author Shinjiman
 */
$messages['zh-hant'] = array(
	'pfunc_desc'                            => '用邏輯功能去加強處理器',
	'pfunc_time_error'                      => '錯誤: 不正確的時間',
	'pfunc_time_too_long'                   => '錯誤: 過多 #time 的呼叫',
	'pfunc_rel2abs_invalid_depth'           => '錯誤: 不正確的路徑深度: "$1" (已經嘗試在頂點存取該點)',
	'pfunc_expr_stack_exhausted'            => '表達錯誤: 堆疊耗盡',
	'pfunc_expr_unexpected_number'          => '表達錯誤: 未預料的數字',
	'pfunc_expr_preg_match_failure'         => '表達錯誤: 未預料的 preg_match失敗',
	'pfunc_expr_unrecognised_word'          => '表達錯誤: 未預料的字 "$1"',
	'pfunc_expr_unexpected_operator'        => '表達錯誤: 未預料的 $1 運算符',
	'pfunc_expr_missing_operand'            => '表達錯誤: 缺少 $1 的運算符',
	'pfunc_expr_unexpected_closing_bracket' => '表達錯誤: 未預料的關括號',
	'pfunc_expr_unrecognised_punctuation'   => '表達錯誤: 未能認得到的標點 "$1"',
	'pfunc_expr_unclosed_bracket'           => '表達錯誤: 未關閉的括號',
	'pfunc_expr_division_by_zero'           => '除以零',
	'pfunc_expr_invalid_argument'           => '$1的無效參數: < -1 or > 1',
	'pfunc_expr_invalid_argument_ln'        => 'ln的無效參數: <= 0',
	'pfunc_expr_unknown_error'              => '表達錯誤: 未知嘅錯 ($1)',
	'pfunc_expr_not_a_number'               => '於 $1: 結果不是數字',
);

