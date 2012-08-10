#include <string.h>
#include <stdio.h>

// project includes
#include "win32wm.h"
#include "resource.h"
// The registry key of Win32WM
#define WIN32WM_REG_KEY "Software\\Win32WM\\Settings"

// Register the hotkey atoms used by the program
static void RegisterHotKeyAtoms()
{
	atomVertical     = GlobalAddAtom( "MaximizeVertically" );
	atomHorizontal   = GlobalAddAtom( "MaximizeHorizontally" );
	atomMaximize     = GlobalAddAtom( "Maximize" );
	atomMinimize     = GlobalAddAtom( "Minimize" );
	atomBackground   = GlobalAddAtom( "Background" );
}

// unregister the hotkey atoms
void UnRegisterHotKeyAtoms()
{
	UnregisterHotKey( hwndMain, atomVertical );
	UnregisterHotKey( hwndMain, atomHorizontal );
	UnregisterHotKey( hwndMain, atomMaximize );
	UnregisterHotKey( hwndMain, atomMinimize );
	UnregisterHotKey( hwndMain, atomBackground );
}

// Open the registry key that holds the application information
HKEY GetConfigRegKey()
{
	HKEY key = NULL;
	RegCreateKey( HKEY_CURRENT_USER, WIN32WM_REG_KEY, &key );
	return key;
}

/** 
 * Save the config. 
 * Writes an double word to registry with all on/off setting.
 */
BOOL DumpConfig()
{
	DWORD data = 0;
	HKEY key;

	data =   (useDrag?1:0)         
		   | (useVertical?2:0)     
		   | (useHorizontal?4:0)   
		   | (useResize?8:0)       
		   | (useKDEResize?0x10:0) 
		   | (snap?0x20:0)         
		   | (useMaximize?0x40:0)  
		   | (useMinimize?0x80:0)  
		   | (useBackground?0x10000:0)
		   | (useMDI?0x20000:0)
		   | (useAltBackground?0x40000:0);
	data |= (snapToAt << 8);

	key = GetConfigRegKey();
	if( key == NULL )
	{
		return FALSE;
	}

	if( RegSetValueEx( key, "Settings", 0, REG_DWORD, (LPBYTE) &data, sizeof( DWORD ) ) != ERROR_SUCCESS )
		MessageBox( hwndMain, "Saving settings to registry failed", "Win32WM Error", MB_OK | MB_ICONERROR );

	return TRUE;
}

/**
 * Parse the hotkey modifiers from a string. 
 * Assumes the string is in all uppercase
 */
UINT GetHotkeyModifiers( char *keyString )
{
	UINT result = 0;
	if( strstr( keyString, "ALT" ) )
		result |= MOD_ALT;
	if( strstr( keyString, "SHIFT" ) )
		result |= MOD_SHIFT;
	if( strstr( keyString, "CONTROL" ) )
		result |= MOD_CONTROL;
	if( strstr( keyString, "WIN" ) )
		result |= MOD_WIN;
	return result;
}

/**
 * Parse the hotkey character out of the string. 
 * Destroys the string in the process
 */
UINT GetHotkeyChar( char *keyString )
{
	UINT result = 0;
	char *p;
	int i;

	keyString[ strlen( keyString ) + 1 ] = 0;
	for( i=0; keyString[i] != 0; i++ )
	{
		if( keyString[i] == '+' )
			keyString[i] = 0;
	}

	for( p = keyString; p != NULL; p++ )
	{
		if( strlen( p ) == 1 )
		{
			result = (UINT) *p;
			break;
		}
		else
			while( *(++p) );
	}

	return result;
}

void CreateHotkeyRegKey( char *regKey, char *regValue )
{
	HKEY key = GetConfigRegKey();

	if( RegSetValueEx( key, regKey, 0, REG_SZ, (LPBYTE) regValue, strlen( regValue ) ) != ERROR_SUCCESS )
		MessageBox( hwndMain, "Saving settings to registry failed", "Win32WM Error", MB_OK | MB_ICONERROR );
}

/* Convert string to uppercase in place */
void StrToUpper( char *string )
{
	char *p;
	for( p=string; *p; p++ )
		*p = toupper( *p );
}

BOOL RegisterSingleHotkey( ATOM atom, char* regKey, char *errorMsg, char *defaultKey )
{
	HKEY key = GetConfigRegKey();
	char hotkeyString[64];
	DWORD type, 
		  len = sizeof( hotkeyString );

	UINT modifiers = 0;
	UINT hotkeyChar;
	BOOL result = TRUE;
	
	if( RegQueryValueEx( key, regKey, 0, &type, (LPBYTE) &hotkeyString, &len ) != ERROR_SUCCESS )
	{
		// key probably doesn't exist, create the key with default value
		CreateHotkeyRegKey( regKey, defaultKey );
		strcpy( hotkeyString, defaultKey );
	}

	StrToUpper( hotkeyString );
	modifiers = GetHotkeyModifiers( hotkeyString );
	hotkeyChar = GetHotkeyChar( hotkeyString );

	if( !RegisterHotKey( hwndMain, atom, modifiers, hotkeyChar ) )
	{
		char msg[128];

		sprintf( msg, "Unable to register hotkey for %s", errorMsg );
		MessageBox( hwndMain, msg, "Win32WM Error", MB_OK | MB_ICONEXCLAMATION );

		result = FALSE;
	}

	return result;
}

BOOL RegisterAllHotkeys()
{
	struct _HOTKEYCONST
	{ 
		ATOM atom;          // the atom of the hotkey
		char *registryKey;  // the registry key holding the hotkey 
		char *errorMessage; // the message to display if registering fails
		char *defaultHotkey;// default value 
		BOOL enabled;		// enabled
	} hotkeyConstants[] = 
	{
		{ atomVertical,     "MaximizeVertical",   "Maximize Vertically",   "WIN+V", useVertical },
		{ atomHorizontal,   "MaximizeHorizontal", "Maximize Horizontally", "WIN+H", useHorizontal },
		{ atomMaximize,		"Maximize",			  "Maximize",			   "WIN+X", useMaximize },
		{ atomMinimize,		"Minimize",			  "Minimize",              "WIN+Z", useMinimize },
		{ atomBackground,	"SendToBackground",	  "Send To background",    "WIN+B", useBackground }
	};

	BOOL result = TRUE;
	int i;

	for( i=0; i< sizeof( hotkeyConstants ) / sizeof( _HOTKEYCONST ) ; i++ )
	{
		if(hotkeyConstants[i].enabled)
			result &= RegisterSingleHotkey( hotkeyConstants[i].atom, hotkeyConstants[i].registryKey,
						hotkeyConstants[i].errorMessage, hotkeyConstants[i].defaultHotkey );
	}
	
	return result;
}

// loads the config from registry
BOOL LoadConfig()
{
	HKEY key;
	DWORD data;
	DWORD type, len = sizeof( data );
	BOOL result = TRUE;

	key = GetConfigRegKey();

	if( RegQueryValueEx( key, "Settings", 0, &type, (LPBYTE) &data, &len ) != ERROR_SUCCESS )
	{
		/* probably first-time startup, store defaults */
		useDrag = TRUE;
		useResize = FALSE;
		useVertical = TRUE;
		useHorizontal = TRUE;
		snap = TRUE;
		snapToAt = 10;

		useMaximize = useMinimize = 
			useBackground = useMDI = 
			useKDEResize = useAltBackground = 0;
		
		DumpConfig();
		result = FALSE;
	}
	else
	{
		useDrag				= (data & 0x1);
		useVertical			= (data & 0x2);
		useHorizontal		= (data & 0x4);
		useResize			= (data & 0x8);
		useKDEResize		= (data & 0x10);
		snap				= (data & 0x20);
		snapToAt			= (data & 0x0000FF00) >> 8;
		useMaximize			= (data & 0x40);
		useMinimize			= (data & 0x80);
		useBackground		= (data & 0x10000);
		useMDI				= (data & 0x20000);
		useAltBackground	= (data & 0x40000);
	}
	
	RegisterHotKeyAtoms();
	RegisterAllHotkeys();

	return result;
}

/* Properties Dialog functions */

struct _HOTKEY_WIDGETS
{
	char *regKey;
	UINT modWin, modCtrl, modAlt, modShift, editChar;
} hotkeyWidgets[] = 
{
	{ "MaximizeVertical", IDC_CHECK_VERTICAL_WIN, IDC_CHECK_VERTICAL_CTRL, 
		IDC_CHECK_VERTICAL_ALT, IDC_CHECK_VERTICAL_SHIFT, IDC_EDIT_VERTICAL },
	{ "MaximizeHorizontal", IDC_CHECK_HORIZONTAL_WIN, IDC_CHECK_HORIZONTAL_CTRL, 
		IDC_CHECK_HORIZONTAL_ALT, IDC_CHECK_HORIZONTAL_SHIFT, IDC_EDIT_HORIZONTAL },
	{ "Maximize", IDC_CHECK_MAXIMIZE_WIN, IDC_CHECK_MAXIMIZE_CTRL, 
		IDC_CHECK_MAXIMIZE_ALT, IDC_CHECK_MAXIMIZE_SHIFT, IDC_EDIT_MAXIMIZE },
	{ "Minimize", IDC_CHECK_MINIMIZE_WIN, IDC_CHECK_MINIMIZE_CTRL, 
		IDC_CHECK_MINIMIZE_ALT, IDC_CHECK_MINIMIZE_SHIFT, IDC_EDIT_MINIMIZE },
	{ "SendToBackground", IDC_CHECK_BACKGROUND_WIN, IDC_CHECK_BACKGROUND_CTRL, 
		IDC_CHECK_BACKGROUND_ALT, IDC_CHECK_BACKGROUND_SHIFT, IDC_EDIT_BACKGROUND }
};


static void SetHotkeyWidgets( HWND hDlg )
{
	HKEY key;
	int i;
	char hotkeyString[64];
	DWORD type, 
		  len;

	key = GetConfigRegKey();
	for( i=0; i<sizeof( hotkeyWidgets ) / sizeof(_HOTKEY_WIDGETS); i++ )
	{
		char buf[16];

		len = sizeof( hotkeyString );
		RegQueryValueEx( key, hotkeyWidgets[i].regKey, 0, &type, (LPBYTE) &hotkeyString, &len );
		StrToUpper( hotkeyString );
		if( strstr( hotkeyString, "WIN" ) )
			SendDlgItemMessage( hDlg, hotkeyWidgets[i].modWin, BM_SETCHECK, BST_CHECKED, 0 );
		if( strstr( hotkeyString, "CONTROL" ) )
			SendDlgItemMessage( hDlg, hotkeyWidgets[i].modCtrl, BM_SETCHECK, BST_CHECKED, 0 );
		if( strstr( hotkeyString, "ALT" ) )
			SendDlgItemMessage( hDlg, hotkeyWidgets[i].modAlt, BM_SETCHECK, BST_CHECKED, 0 );
		if( strstr( hotkeyString, "SHIFT" ) )
			SendDlgItemMessage( hDlg, hotkeyWidgets[i].modShift, BM_SETCHECK, BST_CHECKED, 0 );
		SetWindowLong( GetDlgItem( hDlg, IDC_SNAP_EDIT ), GWL_STYLE,
			GetWindowLong( GetDlgItem( hDlg, IDC_SNAP_EDIT ), GWL_STYLE ) | ES_UPPERCASE );
		
		sprintf( buf, "%c", GetHotkeyChar( hotkeyString ) );
		SetWindowText( GetDlgItem( hDlg, hotkeyWidgets[i].editChar ), buf );
	}
}

static void GetHotkeyWidgets( HWND hDlg )
{
	HKEY key;
	int i;

	key = GetConfigRegKey();
	for( i=0; i<sizeof( hotkeyWidgets ) / sizeof(_HOTKEY_WIDGETS); i++ )
	{
		char buf[64] = "";
		char charBuf[ 16 ];

		if( IsDlgButtonChecked( hDlg, hotkeyWidgets[i].modWin ) == TRUE )
			strcat( buf, "WIN+" );
		if( IsDlgButtonChecked( hDlg, hotkeyWidgets[i].modCtrl ) == TRUE )
			strcat( buf, "CONTROL+" );
		if( IsDlgButtonChecked( hDlg, hotkeyWidgets[i].modAlt ) == TRUE )
			strcat( buf, "ALT+" );
		if( IsDlgButtonChecked( hDlg, hotkeyWidgets[i].modShift ) == TRUE )
			strcat( buf, "SHIFT+" );
		GetWindowText( GetDlgItem( hDlg, hotkeyWidgets[i].editChar ), charBuf, sizeof( charBuf ) );
		if( strlen( charBuf ) )
		{
			// crop the string to 1 character
			charBuf[ 1 ] = 0;
			strcat( buf, charBuf );
			StrToUpper( buf );
			CreateHotkeyRegKey( hotkeyWidgets[i].regKey, buf );			
		}
	}	
}

static void InitializePropertiesDialog( HWND hDlg )
{
	// set the checkboxes to reflect the current state
	if( useDrag )
		SendDlgItemMessage( hDlg, IDC_CHECK_DRAG, BM_SETCHECK, BST_CHECKED, 0 );
	if( useVertical )
		SendDlgItemMessage( hDlg, IDC_CHECK_VERTICAL, BM_SETCHECK, BST_CHECKED, 0 );
	if( useHorizontal )
		SendDlgItemMessage( hDlg, IDC_CHECK_HORIZONTAL, BM_SETCHECK, BST_CHECKED, 0 );
	if( useResize )
		SendDlgItemMessage( hDlg, IDC_CHECK_RESIZE, BM_SETCHECK, BST_CHECKED, 0 );
	if( useKDEResize )
		SendDlgItemMessage( hDlg, IDC_CHECK_RESIZE_RIGHT, BM_SETCHECK, BST_CHECKED, 0 );
	if( snap )
		SendDlgItemMessage( hDlg, IDC_CHECK_SNAP, BM_SETCHECK, BST_CHECKED, 0 );
	if( useMaximize )
		SendDlgItemMessage( hDlg, IDC_CHECK_MAXIMIZE, BM_SETCHECK, BST_CHECKED, 0 );
	if( useMinimize )
		SendDlgItemMessage( hDlg, IDC_CHECK_MINIMIZE, BM_SETCHECK, BST_CHECKED, 0 );
	if( useBackground )
		SendDlgItemMessage( hDlg, IDC_CHECK_BACKGROUND, BM_SETCHECK, BST_CHECKED, 0 );
	if( useMDI )
		SendDlgItemMessage( hDlg, IDC_CHECK_MDI, BM_SETCHECK, BST_CHECKED, 0 );
	if( useAltBackground )
		SendDlgItemMessage( hDlg, IDC_CHECK_ALT_BACKGROUND, BM_SETCHECK, BST_CHECKED, 0 );
	SetWindowLong( GetDlgItem( hDlg, IDC_SNAP_EDIT ), GWL_STYLE,
		GetWindowLong( GetDlgItem( hDlg, IDC_SNAP_EDIT ), GWL_STYLE ) | ES_NUMBER );
	{
		char buf[ 16 ];
		sprintf( buf, "%d", snapToAt );
		SetWindowText( GetDlgItem( hDlg, IDC_SNAP_EDIT ), buf);
	}
	SetHotkeyWidgets( hDlg );
	forceForeground( hDlg );
}

void StorePropertiesFromDialog( HWND hDlg )
{
	useHorizontal   = ( IsDlgButtonChecked( hDlg, IDC_CHECK_HORIZONTAL   ) == TRUE );
	useVertical     = ( IsDlgButtonChecked( hDlg, IDC_CHECK_VERTICAL     ) == TRUE );
	useDrag 		= ( IsDlgButtonChecked( hDlg, IDC_CHECK_DRAG         ) == TRUE );
	useResize       = ( IsDlgButtonChecked( hDlg, IDC_CHECK_RESIZE       ) == TRUE );
	useKDEResize    = ( IsDlgButtonChecked( hDlg, IDC_CHECK_RESIZE_RIGHT ) == TRUE );
	snap            = ( IsDlgButtonChecked( hDlg, IDC_CHECK_SNAP         ) == TRUE );
	useMaximize     = ( IsDlgButtonChecked( hDlg, IDC_CHECK_MAXIMIZE     ) == TRUE );
	useMinimize     = ( IsDlgButtonChecked( hDlg, IDC_CHECK_MINIMIZE     ) == TRUE );
	useBackground   = ( IsDlgButtonChecked( hDlg, IDC_CHECK_BACKGROUND   ) == TRUE );
	useMDI			= ( IsDlgButtonChecked( hDlg, IDC_CHECK_MDI          ) == TRUE );
	useAltBackground= ( IsDlgButtonChecked( hDlg, IDC_CHECK_ALT_BACKGROUND)== TRUE );

	if( !useDrag && !useResize )
	{
		if( threadStatus == DRAG_ON )
		{
			SuspendThread( dragThread );
			threadStatus = DRAG_OFF;
		}
	}
	else
	{
		if( threadStatus != DRAG_ON )
		{
			ResumeThread( dragThread );
			threadStatus = DRAG_ON;
		}
	}

	{
		char buf[16];

		GetWindowText( GetDlgItem( hDlg, IDC_SNAP_EDIT ), buf, 127 );
		snapToAt = atoi( buf );
	}
	GetHotkeyWidgets( hDlg );
	UnRegisterHotKeyAtoms();
	RegisterHotKeyAtoms();
	RegisterAllHotkeys();

	DumpConfig();
}

// The callback function of the properties dialog
LRESULT CALLBACK PropertiesProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			InitializePropertiesDialog( hDlg );
			break;
		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK ) // save changes
			{
				StorePropertiesFromDialog( hDlg );
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			else if( LOWORD(wParam) == IDCANCEL)
				EndDialog( hDlg, LOWORD(wParam) );
			break;
	}
    return FALSE;
}
