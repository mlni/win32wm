#include <windows.h>

/* Hotkey atoms */
extern ATOM 
	atomHorizontal, 
	atomVertical, 
	atomMaximize, 
	atomMinimize, 
	atomBackground;

/* Main window handle */
extern HWND hwndMain;

/* Various settings and flags */
extern BOOL 
	useDrag,						// wether to use window moving
	useResize,						// wether to resize windows
	useVertical,					// use vertical maximizing
	useHorizontal,					// use horizontal maximizing
	useMaximize,					// use maximize hotkey
	useMinimize,					// use minimize hotkey
	useKDEResize,					// wether to resize in one direction only depending on the position of cursor in the window
	useBackground,					// use send-window-to-background(tm) hotkey
	useAltBackground,
	useMDI,							// Handle MDI windows as root-level windows
	configDialogActive;		        // is the config dialog active?

extern int 
	snapToAt;					// snap to desktop borders at X pixels

extern BOOL snap;
extern HANDLE dragThread;					// handle to the mouse handling thread
extern int threadStatus;					// current status of the mouse handling thread

/* Function prototypes */
LRESULT CALLBACK WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK PropertiesProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
DWORD WINAPI MouseDragThread( LPVOID lParam );
void UnRegisterHotKeyAtoms();
BOOL LoadConfig();
BOOL DumpConfig();
void forceBackground( HWND wnd );
void forceForeground( HWND wnd );

#ifdef DEBUG
void ReportError( void );
#endif

// These two constants define the state of the mouse tracking
// thread (suspended/running)
#define DRAG_ON		0
#define DRAG_OFF	1

