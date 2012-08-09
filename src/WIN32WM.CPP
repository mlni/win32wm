// win32wm.cpp : Defines the entry point for the application.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
//  USA.
//

/*
 * Revision history
 * 07.12.2000	Initial release
 *				Version 0.1					Matti
 * 12.12.2000	Added option for resizing windows using Alt-Rightbutton	Matti
 * 13.12.2000	Fixed bug with incorrect handling of autohiding taskbar
 *				and taskbar being in unusual positions
 *				Version 0.2					Matti
 * 18.04.2001	Added Send window to background option.
 *				Settings are now saved directly to registry instead of a
 *				file in VirtuaWin directory, to allow use as a standalone
 *				application without VirtuaWin
 *				Version 0.3					Matti
 * 24.04.2001	Added support for MDI child window handling. Maximizing in one
 *				direction only now saves the window position and restores it when necessary.
 *				Version 0.4					Matti
 * 10.07.2002	Moved configuration-related code into a separate file config.cpp. Made the
 *              hotkey strings configurable through registry. Changed startup behaviour
 *              to kill any previous win32wm process and show configuration dialog.
 *				Added hotkey configuration to configuration dialog.
 *              Version 0.5                 Matti
 */


#include "stdafx.h"
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include "resource.h"
#include "messages.h"
#include "win32wm.h"


// Globals
HWND vwHandle;
HWND hwndMain;
HINSTANCE hInst;
// atoms for hot keys
ATOM atomHorizontal, atomVertical, atomMaximize, atomMinimize, atomBackground;
HANDLE dragThread;					// handle to the mouse handling thread
BOOL useDrag,						// wether to use window moving
	useResize,						// wether to resize windows
	useVertical,					// use vertical maximizing
	useHorizontal,					// use horizontal maximizing
	useMaximize,					// use maximize hotkey
	useMinimize,					// use minimize hotkey
	useKDEResize,					// wether to resize in one direction only depending on the position of cursor in the window
	useBackground,					// use send-window-to-background(tm) hotkey
	useAltBackground,				// use Alt+middle to send window to background
	useMDI,							// Handle MDI windows as root-level windows
	configDialogActive = FALSE;		// is the config dialog active?
int threadStatus;					// current status of the mouse handling thread
int snapToAt = 5;					// snap to desktop borders at X pixels
BOOL snap;

typedef struct 
{
	LONG winID;
	RECT pos;
} CacheItem;

// The window position cache that holds information necessary for restoreing windows that are
// maximized horizontally or vertically
#define CACHE_SIZE	10
static CacheItem positionCache[CACHE_SIZE];

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	WNDCLASS wnd;
	MSG msg;
	DWORD threadId;
	
	BOOL showConfig = FALSE;

	// Avoid running multiple copies of this thing. One can never know what will happen
	CreateMutex( NULL, FALSE, "Win32WM_DontRunMeTwice" );
	if( GetLastError() == ERROR_ALREADY_EXISTS )
	{
		// Pop up the setup dialog when executed twice
		PostMessage( FindWindow( "win32wm.exe", NULL ), MOD_QUIT, 0, 0 );
		showConfig = TRUE;
		Sleep( 250 );
	}

	// dont ask me what this stuff is for. Things didn't work without registering
	// my own window class, so I registered it.
	memset( &wnd, 0, sizeof(WNDCLASS) );
	wnd.style = 0;
	wnd.lpfnWndProc = (WNDPROC) WndProc;
	wnd.hInstance = hInstance;
	wnd.lpszClassName = "win32wm.exe";
	if( !RegisterClass( &wnd ))
		return -1;

	hInst = hInstance;
	hwndMain = CreateWindow(
			"win32wm.exe", "win32wm", WS_TILEDWINDOW, CW_USEDEFAULT, 0,
			0, 0, NULL, NULL, hInstance, NULL);
	if( !hwndMain )
		return -1;

	LoadConfig();

	// the thread that tracks the movement of mouse and the state of ALT key
	dragThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE) MouseDragThread, NULL, 0, &threadId );
	if( !useDrag && !useResize )
	{
		SuspendThread( dragThread );
		threadStatus = DRAG_OFF;
	}
	else
		threadStatus = DRAG_ON;
	
	if( showConfig )
	{
		PostMessage( hwndMain, MOD_SETUP, 0, 0 );
	}

	// Enter the message loop
	while( GetMessage( &msg, NULL, 0, 0 ) )
	{
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}
	return msg.wParam;
}

BOOL IsWindowInCache( LONG winId )
{
	int i;
	for( i = 0; i < CACHE_SIZE; i++ )
		if( positionCache[i].winID == winId )
			return true;
	return false;
}

RECT* GetWindowPosFromCache( LONG winId )
{
	int i;
	for( i = 0; i < CACHE_SIZE; i++ )
		if( positionCache[i].winID == winId )
			return &(positionCache[i].pos);
	return NULL;
}

void PutWindowPosIntoCache( LONG winId, RECT *pos )
{
	static int lastInsert;
	if( !IsWindowInCache( winId ) )
	{
		lastInsert = (++lastInsert % CACHE_SIZE);
		positionCache[ lastInsert ].winID = winId;
		positionCache[ lastInsert ].pos = *pos;
	}
	else
	{
		int i;
		for( i = 0; i < CACHE_SIZE; i++ )
			if( positionCache[i].winID == winId )
			{
				positionCache[i].pos = *pos;
			}
	}
}
/**
 * This function fills one and only one member of
 * a rect struct with the current height or width
 * of the taskbar window
 */
void GetTaskbarPos( RECT *out )
{
	RECT r, desktop;

	GetWindowRect( FindWindow( "Shell_traywnd", "" ), &r );
	GetWindowRect( GetDesktopWindow(), &desktop );
	memset( out, 0, sizeof( RECT ) );
	if( r.right - r.left >= desktop.right - desktop.left)
	{
		if( r.top<=0 )
			out->top = (r.bottom>2?r.bottom-r.top-2:2);
		else
			out->bottom = (r.top<desktop.bottom-3?r.bottom-r.top-2:2);
	}
	else
	{
		if( r.left<=0 )
			out->left=(r.right>3?r.right-r.left-2:2);
		else
			out->right=(r.left<desktop.right-3?r.right-r.left-2:2);
	}
}

LRESULT CALLBACK WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
	case MOD_INIT:
		vwHandle = (HWND) wParam;
		break;
	case MOD_SETUP:
		if( !configDialogActive )
		{
			configDialogActive = TRUE;
			DialogBox( hInst, MAKEINTRESOURCE( IDD_PROPDIALOG ), hwndMain, (DLGPROC) PropertiesProc );
			configDialogActive = FALSE;
		}
		break;
	case MOD_QUIT:
		PostQuitMessage( 0 );
		break;
	case WM_DESTROY:
		UnRegisterHotKeyAtoms();
		if( threadStatus != DRAG_OFF )
		{
			SuspendThread( dragThread );
			threadStatus = DRAG_OFF;
		}
		PostQuitMessage( 0 );
		break;
	case WM_HOTKEY:
		// Vertical maximize
		if( useVertical && wParam == atomVertical )
		{
			int height, top;
			RECT r;
			RECT taskBarRect = { 0, 0, 0, 0 };

			GetWindowRect( GetDesktopWindow(), &r );
			height = r.bottom - r.top;

			top = 0;
			GetTaskbarPos( &taskBarRect );
			if( taskBarRect.top||taskBarRect.bottom )
			{
				top = taskBarRect.top;
				height -= (taskBarRect.top | taskBarRect.bottom);
			}
			GetWindowRect( GetForegroundWindow(), &r );
			
			LONG id = GetWindowLong( GetForegroundWindow(), GWL_ID );
			if( r.bottom - r.top == height )
			{ // Window already vertically maximized
				if( IsWindowInCache( id ) )
				{
					RECT *prevPos = GetWindowPosFromCache( id );
					SetWindowPos( GetForegroundWindow(), 0, prevPos->left, prevPos->top, 
						prevPos->right - prevPos->left, prevPos->bottom-prevPos->top, 0 );
				}
			}
			else
			{ // put window coords into cache
				PutWindowPosIntoCache( id, &r );
				SetWindowPos( GetForegroundWindow(), 0, r.left, top, r.right - r.left, height, 0 );
			}

		}
		// Horizontal maximize
		else if( useHorizontal && wParam == atomHorizontal )
		{
			int width, left;
			RECT r;
			RECT taskBarRect;
			HWND foreGnd = GetForegroundWindow();

			GetWindowRect( GetDesktopWindow(), &r );
			GetTaskbarPos( &taskBarRect );

			width = r.right - r.left;
			left = 0;
			if( taskBarRect.left || taskBarRect.right )
			{
				left = taskBarRect.left;
				width -= (taskBarRect.right|taskBarRect.left);
			}
			GetWindowRect( foreGnd, &r );
			LONG id = GetWindowLong( foreGnd, GWL_ID );

			if( r.right - r.left == width )
			{
				if( IsWindowInCache( id ) )
				{
					RECT *prev = GetWindowPosFromCache( id );
					SetWindowPos( foreGnd, 0, prev->left, prev->top, prev->right-prev->left, prev->bottom-prev->top, 0 );
				}
			}
			else
			{
				PutWindowPosIntoCache( id, &r );
				SetWindowPos( foreGnd, 0, left, r.top, width, r.bottom-r.top, 0 );
			}
			
		}
		// simple maximize
		else if( useMaximize && wParam == atomMaximize )
		{
			if( GetWindowLong( GetForegroundWindow(), GWL_STYLE ) & WS_MAXIMIZE	)
				PostMessage( GetForegroundWindow(), WM_SYSCOMMAND, SC_RESTORE, 0 );
			else
				PostMessage( GetForegroundWindow(), WM_SYSCOMMAND, SC_MAXIMIZE, 0 );
		}
		// simple minimize
		else if( useMinimize && wParam == atomMinimize )
		{
			PostMessage( GetForegroundWindow(), WM_SYSCOMMAND, SC_MINIMIZE, 0 );
		}
		// send a window to background
		else if( useBackground && wParam == atomBackground )
			forceBackground( GetForegroundWindow() );
	default:
		return DefWindowProc( hwnd, msg, wParam, lParam );
	}
	return 0;
}

/*************************************************
 * Forces a window into the foreground. Must be done in this way to avoid
 * the flashing in the taskbar insted of actually changing active window.
 * This function is copied directly from VirtuaWin (c) 1999, 2000 Johan Piculell
 */
void forceForeground(HWND theWin)
{
   DWORD ThreadID1;
   DWORD ThreadID2;

   if(theWin == GetForegroundWindow()) {
      return;
   } else {
      ThreadID1 = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
      ThreadID2 = GetWindowThreadProcessId(hwndMain, NULL);
      if(ThreadID1 != ThreadID2) {
         AttachThreadInput(ThreadID1, ThreadID2, TRUE);
         SetForegroundWindow(hwndMain);
         AttachThreadInput(ThreadID1, ThreadID2, FALSE);
         SetForegroundWindow(theWin);
      } else {
         SetForegroundWindow(theWin);
      }
   }
}

// Forces the topmost window to the bottom of the Z-axis stack
void forceBackground( HWND wnd )
{
	HDWP wnds = BeginDeferWindowPos( 10 );
	wnds = DeferWindowPos( wnds, wnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE );
	EndDeferWindowPos( wnds );
}


#ifdef DEBUG
// a handy function to grab the lasterror and display some explanation
void ReportError( void )
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0,
		NULL
	);
	MessageBox( NULL, (LPCTSTR)lpMsgBuf, "Win32WM Error", MB_OK | MB_ICONINFORMATION );
	LocalFree( lpMsgBuf );
}
#endif

HWND GetTopmostWindow( POINT *pt )
{
	if( !useMDI )
		return GetForegroundWindow();

	HWND aWnd = WindowFromPoint( *pt );
	while( aWnd != NULL )
	{
		LONG exStyle = GetWindowLong( aWnd, GWL_EXSTYLE );
		if( exStyle & WS_EX_MDICHILD )
		{
			if( GetWindowLong( aWnd, GWL_STYLE  ) & WS_MAXIMIZE )
				return GetForegroundWindow();
			else
			{
				return aWnd;
			}
		}
		aWnd = GetParent( aWnd );
	}
	return GetForegroundWindow();
}

BOOL IsMDIWindow( HWND aWnd )
{
	if( !useMDI )
		return FALSE;
	else
		return ( GetWindowLong( aWnd, GWL_EXSTYLE ) & WS_EX_MDICHILD );
}

// the thread that tracks the mouse button and Alt key states and moves
// the window when necessary. this thread gets suspended when the dragging
// is not allowed from the setup dialog, so there is no need to check the
// global variable and there are no synchronization problems
DWORD WINAPI MouseDragThread( LPVOID lParam )
{
	POINT pt;
	POINT prevPoint;
	BOOL dragging = FALSE, resizing = FALSE;
	BOOL resizeLeft, resizeTop, resizeHorizontal, resizeVertical;
	int alt;
	HWND foregnd;
	BOOL snapTo = TRUE;
	RECT winCoords, snappedTo = {0, 0, 0, 0};

	while( 1 )
	{
		GetCursorPos( &pt );
		foregnd = GetTopmostWindow( &pt );
		// clear the key states
		GetAsyncKeyState( VK_MENU );
		GetAsyncKeyState( VK_LBUTTON );
		GetAsyncKeyState( VK_RBUTTON );
		GetAsyncKeyState( VK_MBUTTON );
		GetAsyncKeyState( VK_SHIFT );
		// is the alt key pressed
		alt = GetAsyncKeyState( VK_MENU ) && !GetAsyncKeyState( VK_SHIFT );
		if( !resizing && useDrag && alt && GetAsyncKeyState( VK_LBUTTON ) )
		{ // Let's rock
			if( !dragging )
			{ // haven't started yet
				dragging = TRUE;
				prevPoint = pt;
				GetWindowRect( foregnd, &winCoords);
				// PostMessage( foregnd, WM_MOUSEMOVE, 0, 0 );
				// PostMessage( foregnd, WM_LBUTTONUP, 0, 0 );
			}
			else
			{ // check whether mouse moved and move window
				if( pt.x != prevPoint.x || pt.y != prevPoint.y )
				{
					RECT r, desk;
					int left, top;
					RECT oldSnaps = snappedTo;

					GetWindowRect( foregnd, &r );
					
					// need  to check desktop size every time, because the taskbar can be auto hiding
					if( !IsMDIWindow( foregnd ) )
						GetWindowRect( GetDesktopWindow(), &desk );
					else
					{
						GetWindowRect( GetParent( foregnd ), &desk );
					}

					if( snap )
					{
						if( !snappedTo.left )
						{
							if( !snappedTo.right )
							{
								if( r.right>=desk.right-snapToAt && r.right< desk.right )
								{
									left = desk.right-(r.right-r.left);
									winCoords.left = r.left;
									snappedTo.right = TRUE;
								}
								else
									left = r.left + pt.x - prevPoint.x;
							}
							else
							{
								winCoords.left += pt.x - prevPoint.x;
								if( winCoords.left+(r.right-r.left) < desk.right-snapToAt || winCoords.left+(r.right-r.left) > desk.right )
								{
									snappedTo.right = FALSE;
									left = winCoords.left;
								}
							}
						}

						if( !snappedTo.right && !oldSnaps.right )
						{
							if( !snappedTo.left )
							{
								if( r.left <= (desk.left + snapToAt) && r.left >= desk.left )
								{
									left = desk.left;
									winCoords.left = r.left;
									snappedTo.left = TRUE;
								}
								else
									left = r.left + pt.x - prevPoint.x;
							}
							else
							{
								winCoords.left += pt.x - prevPoint.x;
								if( winCoords.left > (desk.left+snapToAt) || winCoords.left<desk.left ) 
								{
									snappedTo.left =FALSE;
									left = winCoords.left;
								}
							}
						}


						if( !snappedTo.bottom )
						{
							if( !snappedTo.top )
							{
								if( r.top <= (desk.top+snapToAt) && r.top > desk.top)
								{
									top = desk.top;
									winCoords.top = r.top;
									snappedTo.top = TRUE;
								}
								else
									top = r.top + pt.y - prevPoint.y;
							}
							else
							{
								winCoords.top += pt.y - prevPoint.y;
								if( winCoords.top > (desk.top+snapToAt) || winCoords.top<desk.top)
								{
									snappedTo.top = FALSE;
									top = winCoords.top;
								}
							}
						}

						if( !snappedTo.top  && !oldSnaps.top )
						{
							if( !snappedTo.bottom )
							{
								if( r.bottom>=desk.bottom-snapToAt && r.bottom<desk.bottom)
								{
									top = desk.bottom-(r.bottom-r.top);
									winCoords.top = r.top;
									snappedTo.bottom = TRUE;
								}
								else
									top = r.top + pt.y - prevPoint.y;
							}
							else
							{
								winCoords.top += pt.y - prevPoint.y;
								if( winCoords.top+(r.bottom-r.top) < desk.bottom-snapToAt || winCoords.top+(r.bottom-r.top)>desk.bottom )
								{
									snappedTo.bottom = FALSE;
									top = winCoords.top;
								}
							}
						}
					}
					else
					{
						top = r.top + pt.y - prevPoint.y;
						left = r.left + pt.x - prevPoint.x;
					}

					if( !IsMDIWindow( foregnd ) )
						SetWindowPos( foregnd, 0, left, top,
							r.right - r.left, r.bottom - r.top, 0 );
					else
					{
						/*
						 * OK, the point with the MDI windows is that they must be positioned using the
						 * client area coordinates of the window. This means that it is impossible to
						 * get the window coordinates using GetWindowRect and position the window
						 * using SetWindowPos, because the coordinate 0,0 of the MDI window is the 
						 * top-left corner of the parent window client area. And there is no easy way to
						 * translate the rectangle of the client area into screen coordinates. So I
						 * use a hack to determine the border width of the window and translate the
						 * coordinates myself.
						 *		Matti
						 */
						HWND parentWnd = GetParent( foregnd );  // GetForegroundWindow();
						RECT clientRect, parentRect;
						int borderWidth; 

						GetWindowRect( parentWnd, &parentRect );
						GetClientRect( parentWnd, &clientRect );
						borderWidth = ((parentRect.right - parentRect.left - clientRect.right) / 2);

						if( parentRect.bottom-parentRect.top-clientRect.bottom > 7 
							|| parentRect.right-parentRect.left-clientRect.right > 7) // there's a scrollbar visible in MDI parent window
						{
							SetWindowPos( foregnd, 0, left - parentRect.left, 
								top - parentRect.top ,
								r.right - r.left, r.bottom - r.top, 0 );
						}
						else
						{
							SetWindowPos( foregnd, 0, left - parentRect.right + clientRect.right + borderWidth,
								top - parentRect.bottom + clientRect.bottom + borderWidth, 
								r.right - r.left, r.bottom - r.top, 0 );
						}
					}
					prevPoint = pt;
				}
			}
		}
		else
			dragging = FALSE;

		if( !dragging && useResize && alt && GetAsyncKeyState( VK_RBUTTON ) )
		{
			// Hmm, I wonder if clever design is a good idea when hacking...
			if( !resizing )
			{
				RECT r;

				resizing = TRUE;
				prevPoint = pt;
				// the direction of resizing depends on the place where it starts
				GetWindowRect( foregnd, &r );

				resizeLeft = resizeTop = resizeHorizontal = resizeVertical = FALSE;
				if( pt.x < (r.right+r.left)/2 )
					resizeLeft = TRUE;
				if( pt.y < (r.bottom+r.top)/2 )
					resizeTop = TRUE;
				if( !useKDEResize)
				{
					int x3, x23, y3, y23;
					x3 = ((2*r.left)+r.right)/3;
					x23 = (2*r.right+r.left)/3;
					y3 = ((2*r.top)+r.bottom)/3;
					y23 = (2*r.bottom+r.top)/3;
					if( pt.x <= x3
						|| pt.x >= x23
						|| ( pt.x>x3 && pt.x<x23 && pt.y>y3 && pt.y<y23) )
						resizeHorizontal = TRUE;
					if( pt.y <= y3
						|| pt.y >= y23 )
						resizeVertical = TRUE;
				}
				else
					resizeVertical = resizeHorizontal = TRUE;
				PostMessage( foregnd, WM_NCRBUTTONUP, 0, 0 );
			}
			else if( pt.x != prevPoint.x || pt.y != prevPoint.y )
			{
				int top, bottom, left, right;
				RECT r;

				GetWindowRect( foregnd, &r );
				// initial new values and change only the ones that should be changed
				top = r.top;
				bottom = r.bottom;
				left = r.left;
				right = r.right;
				if( resizeHorizontal )
				{
					if( resizeLeft )
						left = left - prevPoint.x + pt.x;
					else
						right = right - prevPoint.x + pt.x;
				}
				if( resizeVertical )
				{
					if( resizeTop )
						top = top - prevPoint.y + pt.y;
					else
						bottom = bottom - prevPoint.y + pt.y;
				}

				prevPoint = pt;
				if( !IsMDIWindow( foregnd ) )
					SetWindowPos( foregnd, 0, left, top, right-left, bottom-top, 0 );
				else
				{
					HWND parentWnd = GetParent( foregnd ); // GetForegroundWindow();
					RECT clientRect, parentRect;
					int borderWidth; 

					GetWindowRect( parentWnd, &parentRect );
					GetClientRect( parentWnd, &clientRect );

					borderWidth = ((parentRect.right - parentRect.left - clientRect.right) / 2);

					SetWindowPos( foregnd, 0, left - parentRect.right + clientRect.right + borderWidth, 
						top - parentRect.bottom + clientRect.bottom + borderWidth, 
						right-left, bottom-top, 0 );
				}
			}
		}
		else
			resizing = FALSE;

		if( useAltBackground && alt && GetAsyncKeyState( VK_MBUTTON ) )
		{
			forceBackground( foregnd );
		}

		if( dragging )
			Sleep( 10 );
		else
			Sleep( 100 );
	}
}
