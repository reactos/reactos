Desktop Example
---------------

This program doesn't do much, apart from create a window which could be
used as a desktop.

It's pretty straightforward. It creates a window the size of the screen,
displays some text in the corner, and then disables ALT+F4.

Ideally, this would be incorporated into some other part of ReactOS, where
it could be closed in a controlled manner (ie, when the user wishes to exit
the GUI.)

Hope someone finds it of some use. I think it should run before the
explorer clone (taskbar) to get the wallpaper displayed (since when
explorer crashes on Windows, the wallpaper is always displayed, and there
is always a desktop, even with no icons, when the login window is shown.)

It obviously is in need of some improvement, such as wallpaper actually
being drawn (stretch, center/centre and tile...)

So, feel free to play around with it.

Andrew "Silver Blade" Greenwood
silverblade_uk@hotmail.com


Explorer Bar Example
--------------------

I have merged in Alexander Ciobanu's Explorer bar code as a example starting
for the start menu. Its very simple at this point and just loads a window with 
buttons. 

The loading of this module was based on a patch by Martin Fuchs.

Steven Edwards 
Steven_Ed4153@yahoo.com
