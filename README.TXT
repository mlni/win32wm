
Win32WM Overview and Usage
--------------------------
Win32WM is a VirtuaWin module that handles a couple of window manager
functions present in most modern Unix WindowManagers I missed most.
It allows moving windows around by pressing and holding the ALT key 
and dragging the window using the left mouse button. Like Unix WMs
it allows resizing windows by pressing ALT key and dragging the right
mouse button. Unlike the common behaviour on Unix, it also allows you
to move the MDI windows around, which makes a bit more sense with 
apps like Opera, which are often used in maximized state with a few 
of MDI child windows, where moving or resizing the whole parent window 
just doesn't make a lot of sense. 

It can also maximize windows in one direction only, 
ie only vertically or only  horizontally using by default the 
hotkeys WIN+V and WIN+H. There should be no problem with the 
position of the taskbar, at least I hope that it is handled properly.
If you press WIN+V of WIN+H again with the same window active, then 
the window is restored to it's original size, restoring it's previous
position and size. Actually only the positions of 10 last 
windows are remembered, after which the oldest positions get 
overwritten by new values.

The program was initally intended to be used with VirtuaWin, 
but I noticed that it doesn't depend on VirtuaWin in any way, so
it can also be used as a standalone app. If you run it from within
VirtuaWin as a module, you can use the modules tab of setup dialog 
of VirtuaWin to configure the Win32WM. If you run it as a standalone
app and want to reach the Properties dialog, just run the program
once more and it will kill the running instance of itself (to reload 
fresh changes from registry) and displays the properties dialog.

This little program is free software, it can be redistributed
under the terms of GPL licence. As it is licenced under GNU General
Public Licence, Version 2 or newer, the redistributed copy must also 
include the source code of the program. For more details on the 
licence see Copying.txt, which is included in the distribution.

And BTW, Win32WM stands (or originally stood) for Win32 Window Manager :)

Default keyboard shortcuts
---------------------------
Maximize vertically        Win+V
Maximize horizontally      Win+H
Maximize window            Win+X
Minimize window            Win+Z
Send window to background  Win+B

All shortcut keys are optional and can be enabled/disabled in 
configuration dialog. The configuration dialog can be reached 
by executing the Win32WM twice.
A short overview of all possible settings is provided below, 
in the section "Configuration dialog".

Installation
------------
If you are using VirtuaWin, then copy the included binary, 
Win32WM.exe to your VirtuaWin modules directory and press reload 
button on VirtuaWin Modules tab. 
If you are using the program as a standalone windows application, 
you can copy it somewhere safe and just run it. 

Uninstall
---------
To uninstall the program disable it using the enable/disable
button on Modules tab of Virtuawin and remove the binary from 
modules directory or just kill and delete it.


Configuration dialog
--------------------
The configuration dialog box includes the following settings:

* Maximize vertically
  If this setting is on, the hotkey specified on the same line
  is used from maximizing a window to the maximal height of the 
  screen. If the window is already maximized vertically, it is
  restored to its initial size.
* Maximize horizontally
  This checkbox enables the hotkey for horizontal maximization,
  which stretches a window to the width of a screen. If the 
  currently active window is already maximized, it is restored
  to its initial width instead.
* Maximize window
  This checkbox enables the shortcut hotkey for maximizing a
  window. If the foreground window is already maximized then 
  it is restored instead.
* Minimize window
  This setting enables a  shortcut key for minimizing a foreground
  window.
* Background window
  This setting enables the Send window to background hotkey, which
  makes sends the currently active window to the bottom of window
  stack, but leaves it still active. This enables you to send a 
  window behind other windows, but still type into this window,
  which is sometimes handy, e.g. while trying to read help and type 
  something into another window without obscuring the help window.
  Ant it's pretty handy for sending those "Always on top" windows
  to background.
* Use Alt-drag
  This enables the window dragging feature, which lets you use drag
  a window around without reacing its title bar by holding down the
  Alt key and dragging anywhere inside a window.
* Use Alt-resize
  This setting lets you resize any windows by holding the Alt key and
  dragging the right mouse button anywhere inside the window.
* Use Alt-background
  This option lets you use another handy shortcut for sending a 
  window to the background. If this option is selected, just hold Alt
  and click the middle mouse button anywhere inside the window to
  send the window to behind other windows.
* MDI window handling
  This option requires the Alt-drag option to be set. It enables moving
  MDI child windows around using the same Alt+Left mouse button 
  shortcut. This makes it possible for example to drag an Opera
  browser window around inside the Opera window. And as an added 
  bonus it lets you snap the child windows to the parent windows'
  edges.
* Always resize in both directions
  By default when resizing the window using the Alt+Right mouse
  button drag the window is only resized in the direction where
  the mouse cursor is located in respect to the central point of
  the window. For example when you move the mouse to the lower
  middle part of a window and alt-resize it, then only the
  vertical dimension of the window is resized. Accordingly,
  when starting resizing in the middle right part of the window
  then the window is only resized horizontally.
  When this option is enabled, this behaviour is disabled and
  the window is always resized in both horizontal and vertical
  dimensions. This setting requires the Alt-resize feature to 
  be enabled.
* Snap to desktop edges
  This option enables snapping windows to desktop edges when
  they are dragged closer than specified number of pixels from
  an edge of desktop. This only works when dragging a window
  using the Alt-drag option.



For more information on VirtuaWin see http://virtuawin.sourceforge.net/

For more information on Win32WM email me.


    Matti Jagula
    matti.jagula@gmail.com