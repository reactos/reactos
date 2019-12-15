<p align=center>
  <a href="https://reactos.org">
    <img alt="ReactOS" src="https://reactos.org/wiki/images/0/02/ReactOS_logo.png">
  </a>
</p>

---

<p align=center>
  <a href="https://reactos.org/project-news/reactos-0412-released">
    <img alt="ReactOS 0.4.12" src="https://img.shields.io/badge/release-0.4.12-0688CB.svg">
  </a>
  <a href="https://reactos.org/download">
    <img alt="Загрузить ReactOS" src="https://img.shields.io/badge/download-latest-0688CB.svg">
  </a>
  <a href="https://sourceforge.net/projects/reactos">
    <img alt="Загрузить через SourceForge" src="https://img.shields.io/sourceforge/dm/reactos.svg?colorB=0688CB">
  </a>
  <a href="https://github.com/reactos/reactos/blob/master/COPYING">
    <img alt="Лицензия" src="https://img.shields.io/badge/license-GNU_GPL_2.0-0688CB.svg">
  </a>
  <a href="https://reactos.org/donating">
    <img alt="Сделать пожертвование" src="https://img.shields.io/badge/%24-donate-E44E4A.svg">
  </a>
  <a href="https://twitter.com/reactos">
    <img alt="Следите за нами на Twitter" src="https://img.shields.io/twitter/follow/reactos.svg?style=social&label=Follow%20%40reactos">
  </a>
</p>

## Быстрые ссылки
[Сайт](https://reactos.org) &bull; 
[Официальный чат](https://chat.reactos.org) &bull; 
[Вики](https://reactos.org/wiki) &bull; 
[Форум](https://reactos.org/forum) &bull; 
[JIRA Bug Tracker](https://jira.reactos.org/issues) &bull; 
[Зеркало ReactOS(Git)](https://git.reactos.org) &bull; 
[Testman](https://reactos.org/testman/)


## Что такое ReactOS?

ReactOS™ - это проект с открытым исходным кодом для разработки качественной операционной системы, совместимой с приложениями и драйверами, написанными для семейства операционных систем Microsoft® Windows™ NT (NT4, 2000, XP, 2003, Vista, 7).

Проект ReactOS, хоть и в настоящее время ориентирован на совместимость с Windows Server 2003, всегда следит за совместимостью с Windows Vista и будущими выпусками Windows NT.

Код ReactOS распространяется по лицензии [GNU GPL 2.0](https://github.com/reactos/reactos/blob/master/COPYING).

***В настоящее время ReactOS находится в состоянии "Alpha". Это означает, что ReactOS находится в стадии интенсивной разработки, что-то может не работать должным образом и может повредить данные, имеющиеся на вашем жестком диске. Рекомендуется тестировать ReactOS на виртуальной машине или на компьютере без конфиденциальных или критических данных!***

## Сборка

[![appveyor.badge]][appveyor.link] [![travis.badge]][travis.link] [![rosbewin.badge]][rosbewin.link] [![rosbeunix.badge]][rosbeunix.link] [![coverity.badge]][coverity.link]

Для сборки системы настоятельно рекомендуется использовать _ReactOS Build Environment (RosBE)._
Актуальные версии для Windows и для Unix / GNU Linux доступны для загрузки здесь: ["Build Environment"](http://www.reactos.org/wiki/Build_Environment).

В качестве альтернативы можно использовать Microsoft Visual C ++ (MSVC) версии 2010+. Про сборку с MSVC описано здесь: ["Visual Studio или Microsoft Visual C++"](https://www.reactos.org/wiki/CMake#Visual_Studio_or_Microsoft_Visual_C.2B.2B).

### Бинарные файлы

Чтобы собрать ReactOS, вы должны запустить скрипт `configure` в каталоге, в котором вы хотите иметь бинарные файлы. Выберите `configure.cmd` или `configure.sh` в зависимости от вашей системы. Затем запустите `ninja <имя_модуля>`, чтобы собрать нужный модуль, или просто `ninja`, чтобы собрать все модули.

### Загрузочные образы

Чтобы создать образ загрузочного компакт-диска, запустите `ninja bootcd` в директории с бинарными файлами ReactOS. Это создаст образ компакт-диска с именем файла `bootcd.iso`.

См. ["Сборка ReactOS"](http://www.reactos.org/wiki/Building_ReactOS) для дополнительной информации.

Вы всегда можете скачать свежие бинарные сборки загрузочных образов со страницы с ["ежедневными сборками"](https://www.reactos.org/getbuilds/).

## Установка

По умолчанию ReactOS может быть установлена только на компьютере, на котором в качестве активного (загрузочного) раздела используется раздел FAT16 или FAT32.
Раздел, на котором должен быть установлен ReactOS (который может быть или не быть загрузочным разделом), также должен быть отформатирован как FAT16 или FAT32.
Программа установки ReactOS может отформатировать разделы, если это необходимо.

Начиная с версии 0.4.10, ReactOS можно установить с помощью файловой системы BtrFS. Но это экспериментальная особенность и, следовательно, лучше использовать файловую систему FAT.

Чтобы установить ReactOS из дистрибутива загрузочного компакт-диска, распакуйте содержимое архива. Затем запишите образ компакт-диска, загрузитесь с него и следуйте инструкциям.

См. ["Установка ReactOS"](https://www.reactos.org/wiki/Installing_ReactOS) на вики, или [INSTALL](INSTALL) для дополнительной информации.

## Тестирование

Если вы обнаружите ошибку, поищите в JIRA - об этом уже может быть сообщено. Если нет, сообщите об ошибке, предоставляя журналы и как можно больше информации.

См. ["Ошибки в файлах"](https://www.reactos.org/wiki/File_Bugs).

__ВНИМАНИЕ:__ Баг-трекер создан _не_ для обсуждений. Пожалуйста, используйте IRC-канал `#reactos` или наш [форум](https://reactos.org/forum).

## Помощь в разработке  ![prwelcome.badge]

Мы всегда ищем разработчиков! Посмотрите [как внести свой вклад](CONTRIBUTING.md), если вы хотите участвовать.

Вы также можете поддержать ReactOS, [пожертвовав](https://reactos.org/donating)! Мы рассчитываем на то, что наши сторонники будут поддерживать наши сервера и ускорять разработку путем [найма разработчиков](https://reactos.org/node/785).

## Больше информации

ReactOS - это бесплатная операционная система с открытым исходным кодом, основанная на архитектуре Windows,
обеспечение поддержки существующих приложений и драйверов, а также альтернатива текущей доминирующей потребительской операционной системе.

Это не другая оболочка, построенная на Linux, как WINE. Он не пытается и не планирует конкурировать с WINE; фактически, часть пользовательского режима ReactOS почти полностью основана на WINE, и наши две команды тесно сотрудничали в прошлом.

ReactOS также не является «еще одной ОС». Он не пытается быть третьим игроком, как любая другая альтернативная ОС. Люди не обязаны удалять Linux и использовать ReactOS; ReactOS - это замена для пользователей Windows, которым нужна замена Windows, которая ведет себя так же, как Windows.

Более подробная информация доступна по адресу: [reactos.org](https://www.reactos.org).

Также см. Подкаталог [media/doc](/media/doc/) для некоторых разреженных заметок.

## Кто ответственный?

Активные разработчики перечислены в качестве членов [организации GitHub](https://github.com/orgs/reactos/people).
См. также файл [credits](CREDITS) для других.

## Зеркала исходного кода

Основная разработка идёт на [GitHub](https://github.com/reactos/reactos). У нас есть [альтернативное зеркало](https://git.reactos.org/?p=reactos.git) на случай, если GitHub не работает.

Существует также устаревший [архив SVN-хранилища](https://svn.reactos.org/reactos/), который хранится в исторических целях.

[travis.badge]:     https://travis-ci.org/reactos/reactos.svg?branch=master
[appveyor.badge]:   https://ci.appveyor.com/api/projects/status/github/reactos/reactos?branch=master&svg=true
[coverity.badge]:   https://scan.coverity.com/projects/205/badge.svg?flat=1
[rosbewin.badge]:   https://img.shields.io/badge/RosBE_Windows-2.1.6-0688CB.svg
[rosbeunix.badge]:  https://img.shields.io/badge/RosBE_Unix-2.1.2-0688CB.svg
[prwelcome.badge]:  https://img.shields.io/badge/PR-welcome-0688CB.svg

[travis.link]:      https://travis-ci.org/reactos/reactos
[appveyor.link]:    https://ci.appveyor.com/project/AmineKhaldi/reactos
[coverity.link]:    https://scan.coverity.com/projects/205
[rosbewin.link]:    https://sourceforge.net/projects/reactos/files/RosBE-Windows/i386/2.1.6/
[rosbeunix.link]:   https://sourceforge.net/projects/reactos/files/RosBE-Unix/2.1.2/
