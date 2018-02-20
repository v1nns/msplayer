/*****************************************************

Mindstorm MP3 Player

	Version: 1.0
	Date: 7/28/06
	Author: Brandon Sachtleben
	Description: Very simple MP3 player. This is my first
				time working with audio, so although it's
				not perfect, it was just fun for me to
				mess with. I'm sure there are better methods
				than what I used. I wrote the whole thing in
				about an hour or less, so excuse the messy
				code :-).
				Feel free to use the code for
				whatever you want and give any feedback
				you have.

	Version: 2.0
	Date: 10/22/15
	Author: Vinicius M. Longaray
	Description: Using the code I've found on the internet, 
				I took the initiative and made some improvements like
				using global hook to hide and show the player,
				some visual enhancements (it's not that pretty but if it works, that's what matters, right? heheh),
				changed the way how it handles a list of songs
				and created a workaround to step the song to the part you wanna hear.

*****************************************************/

////// Includes
#pragma once
#include <afxwin.h>
#include <shlobj.h>
#include <mmsystem.h>
#include <commctrl.h>
#include <stdio.h>
#include <vector>
#include "resource.h"
#include "MP3FileInfo.h"
#include <string>
#include <vector>
#include <stack>
#include <iostream>
#include <locale>
#include <codecvt>
#include <string>
#include <afxmt.h>

using namespace std;

#define IDC_LISTBOX		1099

////// Globals
HWND hWnd			= NULL;
HWND hPlay			= NULL;
HWND hStop			= NULL;
HWND hPause			= NULL;
HWND hOpen			= NULL;
HWND hProgress		= NULL;
HWND hList			= NULL;
HFONT hFont			= NULL;
HWND hLoadingBar	= NULL;
HANDLE hBitmap		= NULL;
HDC dcSkin			= NULL;
BOOL bDone			= FALSE;
BOOL bRegioned		= TRUE;
int Status			= 0;
int ScreenWidth		= 341;
int ScreenHeight	= 251;//263;
POINT curpoint, point;
RECT MainRect, rect, progress;
HINSTANCE hInst;
MCIDEVICEID pDevice = 0;
MCI_OPEN_PARMS op;
MP3FileInfo lastFile;
CString lastIndexPlayed;
char Title[256];
char Time[256];
char szFileTitle[300];
int hours, minutes, seconds;
int rSeconds;

////// Prototypes
LRESULT CALLBACK WndProc( HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam );
void SkinMe(HDC dc);
void RegionMe();
void ShowMessage( const char *Text, const char *Title, ... );
void ShowMessage( const char *Text, ... );
//void PlayAudio( const char *Filename );
void PlayAudio( MP3FileInfo& song);
void PauseAudio();
void StopAudio();
void StepAudio(int value);

/** Hook **/
HHOOK kbdhook;
CString global_Command;
bool minimized = false;
CMutex* m_pMutex = new CMutex();
CSingleLock lock(m_pMutex);

/* Getting handle from child windows */
WNDPROC g_pfnOldWndProc;
CMutex* m_pMutexHour = new CMutex();
CSingleLock lockHour(m_pMutex);

/** List Box Variables**/
CMap<CString, LPCSTR, MP3FileInfo, MP3FileInfo&> m_Songs;

/** Functions **/
bool ListFilesAddSong(std::vector<string>& out);

/**
 * \brief Called by Windows automagically every time a key is pressed (regardless
 * of who has focus)
 */
__declspec(dllexport) LRESULT CALLBACK handlekeys(int code, WPARAM wp, LPARAM lp)
{
	if (code == HC_ACTION && (wp == WM_SYSKEYDOWN || wp == WM_KEYDOWN)) {
		static bool capslock = false;
		static bool shift = false;
		char tmp[0xFF] = {0};
		CString str;
		DWORD msg = 1;
		KBDLLHOOKSTRUCT st_hook = *((KBDLLHOOKSTRUCT*)lp);

		/*
		 * Get key name as string
		 */
		msg += (st_hook.scanCode << 16);
		msg += (st_hook.flags << 24);
		GetKeyNameText(msg, tmp, 0xFF);
		str = CString(tmp);

		if(str.Compare("Alt") == 0)
			global_Command += str;
		else if(str.Compare("F12") == 0)
		{
			global_Command += str;

			if(global_Command.Find("AltF12") != -1)
			{
				//lock.Lock();
				ShowWindow(hWnd, SW_SHOWDEFAULT);
				minimized = false;
				//lock.Unlock();

				global_Command = "";
			}
			else
			{
				//lock.Lock();
				minimized = true;
				ShowWindow(hWnd, SW_HIDE);
				global_Command = "";
				//lock.Unlock();
			}
		}
		else if(str.Compare("F7") == 0)
		{
			CString temp = lastIndexPlayed;
			int temps = atoi(temp);

			if(temps > 1)
			{
				temps--;
				temp.Format("%02d", temps);

				MP3FileInfo file;
				if(m_Songs.Lookup(temp, file) != 0)
				{
					// Stop audio file
					sprintf( Title, "Stopped: %s", szFileTitle );
					//InvalidateRect(hWnd, &rect, TRUE);
					StopAudio();
					Status = 0;

					SendMessage(hProgress, PBM_SETPOS, 0, 0);

					lastFile = file;
					lastIndexPlayed = temp;
					PlayAudio(file);
					Status = 1;

					SendMessage(hList, LB_SELECTSTRING, 0, (LPARAM)(LPCTSTR)temp);
				}
			}
		}
		else if(str.Compare("F8") == 0)
		{
			CString temp = lastIndexPlayed;
			int temps = atoi(temp);

			temps++;
			temp.Format("%02d", temps);

			MP3FileInfo file;
			if(m_Songs.Lookup(temp, file) != 0)
			{
				// Stop audio file
				sprintf( Title, "Stopped: %s", szFileTitle );
				//InvalidateRect(hWnd, &rect, TRUE);
				StopAudio();
				Status = 0;
				
				SendMessage(hProgress, PBM_SETPOS, 0, 0);

				lastFile = file;
				lastIndexPlayed = temp;
				PlayAudio(file);
				Status = 1;

				SendMessage(hList, LB_SELECTSTRING, 0, (LPARAM)(LPCTSTR)temp);
			}
		}
		else if(str.Compare("F9") == 0)
		{
			//lock.Lock();
			if(Status == 0 || Status == 2)
				SendMessage(hPlay, BM_CLICK, 0, 0);
			else if(Status == 1)
				SendMessage(hPause, BM_CLICK, 0, 0);

			//lock.Unlock();

			global_Command = "";
		}
		else if(str.Compare("F10") == 0)
		{
			if(Status == 1 || Status == 2)
				SendMessage(hStop, BM_CLICK, 0, 0);
		}
		else
		{
			global_Command = "";
		}
	}

	return CallNextHookEx(kbdhook, code, wp, lp);
}

/** char convertion **/
/* Returns a list of files in a directory (except the ones that begin with a dot) */
void GetFilesInDirectory(std::vector<string> &out, const string &directory)
{
    HANDLE dir;
    WIN32_FIND_DATA file_data;

    if ((dir = FindFirstFile((directory + "/*").c_str(), &file_data)) == INVALID_HANDLE_VALUE)
        return; /* No files found */

    do {
        const string file_name = file_data.cFileName;
        const string full_file_name = directory + "/" + file_name;
        const bool is_directory = (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

        if (file_name[0] == '.')
            continue;

        if (is_directory)
		{
			GetFilesInDirectory(out, (directory + "/")+file_name);
            continue;
		}

        out.push_back(full_file_name);
    } while (FindNextFile(dir, &file_data));

    FindClose(dir);
} // GetFilesInDirectory

/** Main **/
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd )
{
	/* hook */
	HINSTANCE	modulehandle;
	modulehandle = GetModuleHandle(NULL);
	kbdhook      = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)handlekeys, modulehandle, NULL);

	/** main **/
	WNDCLASSEX wc;
	MSG Msg;
	
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.hbrBackground = (HBRUSH)GetStockObject( DKGRAY_BRUSH );
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );
	wc.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(IDI_ICON));
	wc.hIconSm = LoadIcon( hInstance, MAKEINTRESOURCE(IDI_ICON));
	wc.hInstance = hInstance;
	wc.lpfnWndProc = WndProc;
	wc.lpszClassName = "AUDIOPLAYER";
	wc.lpszMenuName = NULL;
	wc.style = CS_HREDRAW | CS_VREDRAW;

	// Register window class
	RegisterClassEx( &wc );

	// Create the window
	int x = (GetSystemMetrics(SM_CXSCREEN) / 2) - (ScreenWidth / 2);
	int y = (GetSystemMetrics(SM_CYSCREEN) / 2) - (ScreenHeight / 2);
	hWnd = CreateWindowEx( NULL, "AUDIOPLAYER", "Audio Player", WS_OVERLAPPEDWINDOW,
		x, y, ScreenWidth, ScreenHeight, NULL, NULL, hInstance, NULL );
	ShowWindow( hWnd, SW_SHOW );
	UpdateWindow( hWnd );

	//HBITMAP hSkinBmp = LoadBitmap( hInstance, MAKEINTRESOURCE( IDB_SKIN ));
	dcSkin = CreateCompatibleDC(0);
	//HBITMAP hOldBmp = (HBITMAP)SelectObject( dcSkin, hSkinBmp );
	RegionMe();

	hInst = hInstance;

	// Set timer
	DWORD lastTime = GetTickCount() * 0.001f;
	DWORD curTime;

	// Progress bar
	INITCOMMONCONTROLSEX InitCtrlEx;
	
	InitCtrlEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
	InitCtrlEx.dwICC  = ICC_PROGRESS_CLASS;
	InitCommonControlsEx(&InitCtrlEx);

	// set timer rect
	rect.left = 10;
	rect.right = 320;
	rect.top = 50;
	rect.bottom = 100;

	/* Last song file */
	lastFile = MP3FileInfo();

	bool startNextMusic = false;

	while( !bDone )
	{
		if(PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
		{
			if(Msg.message == WM_QUIT)
				bDone = true;
			else
			{
				TranslateMessage(&Msg); //Translate the message
				DispatchMessage(&Msg);  //Dispatch the message
			}
		}
		else
		{
			if(startNextMusic)
			{			
				CString temp = lastIndexPlayed;
				int temps = atoi(temp);

				temps++;
				temp.Format("%02d", temps);

				MP3FileInfo file;
				if(m_Songs.Lookup(temp, file) != 0)
				{
					lastIndexPlayed = temp;
					PlayAudio(file);
					Status = 1;

					SendMessage(hList, LB_SELECTSTRING, 0, (LPARAM)(LPCTSTR)temp);
				}

				startNextMusic = false;
			}

			// not necessarily the best of way of doing this but it works
			if(Status == 1)
			{
				if( ::PeekMessage(&Msg, hWnd, NULL, NULL, PM_NOREMOVE) )
				{
					if(GetMessage( &Msg, hWnd, NULL, NULL ))
					{
						TranslateMessage( &Msg );
						DispatchMessage( &Msg );
					}
				}

				curTime = GetTickCount() * 0.001f;
				if( curTime - lastTime >= 1 ) // 1 second
				{
					lockHour.Lock();

					SendMessage(hProgress, PBM_SETSTEP, 1, 0);
					SendMessage(hProgress, PBM_STEPIT, 0, 0);
					if(seconds == 1 && minutes == 0 && hours == 0)
					{ // song is done
						sprintf( Title, "Stopped: %s", szFileTitle );
						StopAudio();
						Status = 0;
						SendMessage(hProgress, PBM_SETPOS, 0, 0);

						CString temp = lastIndexPlayed;
						int temps = atoi(temp);

						temps++;
						if(temps <= m_Songs.GetSize())
							startNextMusic = true;

						//lastFile.Free();
					}
					if(seconds != 0)
						seconds--; // subtract 1 second
					else {
						minutes--;
						seconds = 59; // reset
					}
					sprintf(Time, "%02d:%02d:%02d", hours, minutes, seconds);	// update time
					//InvalidateRect(hWnd, &rect, TRUE);
					lastTime = curTime;

					lockHour.Unlock();
				}
			}
		}
	}

	return Msg.wParam;
}

INT CALLBACK BrowseCallbackProc( HWND hwnd, UINT uMsg, 
    LPARAM lp,  LPARAM pData ) 
{
    TCHAR szDir[MAX_PATH];
 
    switch(uMsg) 
    {
        case BFFM_INITIALIZED:
            if ( pData ) wcscpy( (wchar_t*)szDir, (wchar_t *)pData );
            else GetCurrentDirectory(
                sizeof(szDir) / sizeof(TCHAR), szDir);
 
            //selects the specified folder 
            //in the Browse For Folder dialog box
            SendMessage(hwnd, BFFM_SETSELECTION, 
                TRUE, (LPARAM)szDir);
 
            break;
 
        case BFFM_SELCHANGED: 
            if (SHGetPathFromIDList(
                (LPITEMIDLIST) lp, szDir))
            {
                //sets the status text 
                //in the Browse For Folder dialog box
                SendMessage(hwnd, BFFM_SETSTATUSTEXT,
                    0, (LPARAM)szDir);
            }
        break;
    }
    return 0;
}

int count = 0;

LRESULT WINAPI SubclassWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
       case WM_LBUTTONDOWN:
		{
			SetCapture( hProgress );
			GetWindowRect(hProgress, &progress);
      
			//save current cursor coordinate
			GetCursorPos(&point);
     
			ScreenToClient(hProgress, &point);			
			
			if(point.x >= 0 && point.x <= 169)
			{
				if(point.y >= 0 && point.y <= 33)
				{
					if(Status == 1)
					{
						lockHour.Lock();

						double percentage = (point.x / 1.69) / 100; 
						int time = lastFile.nLength * percentage;// * 1000;
						
						SendMessage(hProgress, PBM_SETPOS, time, 0);
						
						seconds = (lastFile.nLength - time)% 60;
						hours = (lastFile.nLength - time)/60/60;
						minutes = (lastFile.nLength - time)/60;

						StepAudio(time);

						lockHour.Unlock();
					}
				}
			}
		}
		break;

	   case WM_LBUTTONUP:
		   ReleaseCapture();
		   break;
	}
	
	return CallWindowProc(g_pfnOldWndProc, hWnd, uMsg, wParam, lParam);
}


LRESULT CALLBACK WndProc( HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam )
{
	switch( Msg )
	{
		case WM_CREATE:
			hPlay = CreateWindow( "BUTTON", "Play", WS_CHILD | WS_VISIBLE | BS_FLAT | BS_BITMAP,
				10, 200, 35, 35, hWnd, NULL, hInst, NULL );
			hPause = CreateWindow( "BUTTON", "Pause", WS_CHILD | WS_VISIBLE | BS_FLAT | BS_BITMAP,
				45, 200, 35, 35, hWnd, NULL, hInst, NULL );
			hStop = CreateWindow( "BUTTON", "Stop", WS_CHILD | WS_VISIBLE | BS_FLAT | BS_BITMAP,
				80, 200, 35, 35, hWnd, NULL, hInst, NULL );
			hOpen = CreateWindow( "BUTTON", "Open", WS_CHILD | WS_VISIBLE | BS_FLAT | BS_BITMAP,
				296, 200, 35, 35, hWnd, NULL, hInst, NULL );

			/* new */
			hList = CreateWindow( "LISTBOX", "List", WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
				10, 10, 320, 190, hWnd, NULL, hInst, (LPVOID)IDC_LISTBOX );

			hFont = CreateFont(16, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial");

			SendMessage(hList, WM_SETFONT, (WPARAM)hFont, 0);

			/* **** */

			hBitmap = LoadImage (GetModuleHandle (NULL), MAKEINTRESOURCE(IDB_BITMAP_PLAY),
				IMAGE_BITMAP,0, 0,LR_DEFAULTCOLOR /*| LR_LOADFROMFILE*/);
			SendMessage(hPlay, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP,
				 (LPARAM)(HANDLE) hBitmap);

			hBitmap = LoadImage (GetModuleHandle (NULL), MAKEINTRESOURCE(IDB_BITMAP_PAUSE),
				IMAGE_BITMAP,0, 0,LR_DEFAULTCOLOR /*| LR_LOADFROMFILE*/);
			SendMessage(hPause, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP,
				(LPARAM)(HANDLE) hBitmap);

			hBitmap = LoadImage (GetModuleHandle (NULL), MAKEINTRESOURCE(IDB_BITMAP_STOP),
				IMAGE_BITMAP,0, 0,LR_DEFAULTCOLOR /*| LR_LOADFROMFILE*/);
			SendMessage(hStop, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP,
				(LPARAM)(HANDLE) hBitmap);

			hBitmap = LoadImage (GetModuleHandle (NULL), MAKEINTRESOURCE(IDB_BITMAP_OPEN),
				IMAGE_BITMAP,0, 0,LR_DEFAULTCOLOR /*| LR_LOADFROMFILE*/);
			SendMessage(hOpen, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP,
				(LPARAM)(HANDLE) hBitmap);

			hProgress = CreateWindowEx(NULL, PROGRESS_CLASS, NULL,
						PBS_SMOOTH | WS_CHILD | WS_VISIBLE,
						120, 200, 171, 35,
						hWnd, NULL, hInst, NULL);
			SendMessage(hProgress, PBM_SETBARCOLOR, 0, (COLORREF)RGB(20, 95, 190));
			SendMessage(hProgress, PBM_SETBKCOLOR, 0, (COLORREF)RGB(210, 230 , 255));

			g_pfnOldWndProc = (WNDPROC) SetWindowLong(hProgress, GWL_WNDPROC, (LPARAM) SubclassWndProc);
			break;

		case WM_COMMAND:
			
			if ((HWND) lParam == hList && HIWORD(wParam) == LBN_DBLCLK) //btn_clicked para list box
			{
				/* verificar se existe algum item selecionado na list box */
				/* Get index of current selection and the text of that selection. */
				int index = SendMessage(hList, LB_GETCURSEL, (WORD)0, 0L);				
				char chBuffer[MAX_PATH];

				DWORD dwSel = SendMessage(hList, LB_GETCURSEL, 0, 0);

				if(dwSel != LB_ERR)
				{
					SendMessage(hList, LB_GETTEXT, dwSel, (LPARAM)(LPSTR)chBuffer);

					CString strMusicSelected = chBuffer;
					
					int lixo = strMusicSelected.Find(".");
					strMusicSelected = strMusicSelected.Mid(0, lixo);

					MP3FileInfo nElement;
					if(m_Songs.Lookup(strMusicSelected, nElement) != 0)
					{
						// Stop audio file
						if(lastFile.szTrack != NULL || Status == 1)
						{
							sprintf( Title, "Stopped: %s", szFileTitle );
							//InvalidateRect(hWnd, &rect, TRUE);
							StopAudio();
							Status = 0;
							SendMessage(hProgress, PBM_SETPOS, 0, 0);
						}

						// Start new audio
						//lastFile = 
						lastIndexPlayed = strMusicSelected;
						PlayAudio(nElement);
						Status = 1;
					}
				}
			}

			if ((HWND)lParam == hPlay && HIWORD(wParam) == BN_CLICKED)
			{
				// Play audio file
				if(lastFile.szTrack != NULL && Status == 0)
				{
					sprintf( Title, "Playing: %s", szFileTitle );
					//InvalidateRect(hWnd, &rect, TRUE);
					PlayAudio( lastFile );
					Status = 1;
				}
				else if(Status == 0 || Status == 1)
				{
					int index = SendMessage(hList, LB_GETCURSEL, (WORD)0, 0L);				
					char chBuffer[MAX_PATH];

					DWORD dwSel = SendMessage(hList, LB_GETCURSEL, 0, 0);

					if(dwSel != LB_ERR)
					{
						SendMessage(hList, LB_GETTEXT, dwSel, (LPARAM)(LPSTR)chBuffer);

						CString strMusicSelected = chBuffer;
							
						int lixo = strMusicSelected.Find(".");
						strMusicSelected = strMusicSelected.Mid(0, lixo);

						MP3FileInfo nElement;
						if(m_Songs.Lookup(strMusicSelected, nElement) != 0)
						{
							CString strToCompare = lastFile.szFilename;
							if(strToCompare.Compare(nElement.szFilename) != 0)
							{
								// Stop audio file
								sprintf( Title, "Stopped: %s", szFileTitle );
								//InvalidateRect(hWnd, &rect, TRUE);
								StopAudio();
								Status = 0;

								SendMessage(hProgress, PBM_SETPOS, 0, 0);

								// Start new audio
								lastFile = nElement;
								lastIndexPlayed = strMusicSelected;
								PlayAudio(nElement);
								Status = 1;
							}
						}
					}
				}
				else if(Status == 2) { // if paused, resume
					mciSendCommand( pDevice, MCI_RESUME, 0, NULL );
					Status = 1;
				}
			}
			
			if ((HWND)lParam == hPause && HIWORD(wParam) == BN_CLICKED)
			{
				// Pause audio file
				if(lastFile.szTrack == NULL) return 0;
				sprintf( Title, "Paused: %s", szFileTitle );
				//InvalidateRect(hWnd, &rect, TRUE);
				PauseAudio();
				Status = 2;
			}

			if ((HWND)lParam == hStop && HIWORD(wParam) == BN_CLICKED)
			{
				// Stop audio file
				if(lastFile.szTrack == NULL) return 0;
				sprintf( Title, "Stopped: %s", szFileTitle );
				//InvalidateRect(hWnd, &rect, TRUE);
				StopAudio();
				Status = 0;
				SendMessage(hProgress, PBM_SETPOS, 0, 0);
			}

			if ( (HWND) lParam == hOpen && HIWORD(wParam) == BN_CLICKED)
			{
				TCHAR szwNewPath[MAX_PATH];
 
				BROWSEINFO bInfo;
				bInfo.hwndOwner = GetActiveWindow();
				bInfo.pidlRoot = NULL; 
				bInfo.pszDisplayName = szwNewPath;
				bInfo.lpszTitle = _T("Select a folder");
 
				bInfo.ulFlags = BIF_NONEWFOLDERBUTTON
					| BIF_NEWDIALOGSTYLE
					| BIF_EDITBOX
					//| BIF_SHAREABLE
					//| BIF_RETURNFSANCESTORS
					| BIF_RETURNONLYFSDIRS;
 
				bInfo.lpfn = BrowseCallbackProc;
				bInfo.lParam = (LPARAM)"C:\\";
 
				LPITEMIDLIST lpItem = SHBrowseForFolder( &bInfo);

				if( lpItem != NULL )
				{
					SHGetPathFromIDList( lpItem, szwNewPath );
					CoTaskMemFree( lpItem );
					
					std::vector<string> out;
					GetFilesInDirectory(out, szwNewPath);

					ListFilesAddSong(out);
				}
			}
			break;

		case WM_KEYDOWN:

			switch(wParam)
			{
				case VK_ESCAPE:
				{
					bDone = TRUE;
				}
					break;
				/*case VK_F12:
				{
					lock.Lock();
					minimized = true;
					ShowWindow(hWnd, SW_HIDE);
					lock.Unlock();
				}
					break;*/
				default:

					break;
			}

			break;

		case WM_LBUTTONDOWN:
		{
			SetCapture( hWnd );
			GetWindowRect(hWnd, &MainRect);
      
			//save current cursor coordinate
			GetCursorPos(&point);
     
			ScreenToClient(hWnd, &point);

			/*// Minimize button
			if(point.x >= 275 && point.x <= 287)
			{
				if(point.y >= 8 && point.y <= 20)
				{
					ShowWindow( hWnd, SW_MINIMIZE );
				}
			}

			// Close window
			if(point.x >= 311 && point.x <= 324)
			{
				if(point.y >= 8 && point.y <= 20)
				{
					SendMessage( hWnd, WM_CLOSE, 0, 0 );
				}
			}*/
		}
			break;

		case WM_LBUTTONUP:
			ReleaseCapture();
			break;

		case WM_MOUSEMOVE:
			GetCursorPos(&curpoint);

			if(wParam==MK_LBUTTON)
			{
				MoveWindow(hWnd, curpoint.x - point.x, curpoint.y - point.y, 
				MainRect.right - MainRect.left, MainRect.bottom - MainRect.top,  
				TRUE);
			}
			break;

		/*case WM_PAINT:
			/*PAINTSTRUCT ps;
			BeginPaint(hWnd,&ps);

			// blit the skin
			//if (bRegioned) SkinMe(ps.hdc);

			SetBkMode( ps.hdc, TRANSPARENT);
			//296, 200, 35, 35
			TextOut( ps.hdc, 10, 240, Title, 45 );
			TextOut( ps.hdc, 10, 250, Time, strlen(Time) );

			EndPaint(hWnd,&ps);
			break;*/

		case WM_DESTROY:
			bDone = TRUE;
			PostQuitMessage( 0 );
			break;
		
	}

	return DefWindowProc( hWnd, Msg, wParam, lParam );
}

bool ListFilesAddSong(std::vector<string>& out)
{
	/* CREATE LOADING BAR */
	/*int x = (GetSystemMetrics(SM_CXSCREEN) / 4) - (ScreenWidth / 4);
	int y = (GetSystemMetrics(SM_CYSCREEN) / 4) - (ScreenHeight / 4);
	
	hLoadingBar = CreateWindowEx( WS_OVERLAPPEDWINDOW, PROGRESS_CLASS, "Player", WS_OVERLAPPEDWINDOW,
		x, y, ScreenWidth, ScreenHeight, NULL, NULL, NULL, NULL );

	//hLoadingBar = CreateWindowEx(0, PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE | PBS_SMOOTH, 20, 20, 260, 17,
	//		      hWndDlg, NULL, hInst, NULL);*/

	int count = m_Songs.GetSize();
	int quantity = out.capacity() + count;

	count++;

	SendMessage(hLoadingBar, PBM_SETRANGE, 0, MAKELPARAM(count, quantity)); 

	for(std::vector<string>::iterator it = out.begin(); it != out.end(); ++it) 
	{
		CString temp = CString(it->c_str());

		if(temp.Find(".mp3") >= 0)
		{
			MP3FileInfo song;
			song.Init(temp);

			CString index;
			index.Format("%02d", count);

			m_Songs.SetAt(index, song);

			/* name file */
			CString name_temp, name_file;

			name_file = song.szFilename;
			name_file = name_file.MakeReverse();

			int position = name_file.Find("/");

			name_file = name_file.Mid(0, position);
			name_file = name_file.MakeReverse();

			position = name_file.Find(".mp3");
			name_file = name_file.Mid(0, position);

			name_file.Trim("0123456789");
			name_file.TrimLeft(" -.");
			/* end name file */
			name_temp.Format("%s. %s", index, name_file);//%s - %s", index, song.szTitle, song.szArtist); 
			SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)name_temp);

			count++;
			SendMessage(hLoadingBar, PBM_SETRANGE, 0, MAKELPARAM(count, quantity)); 
		}
	}

	/*EndDialog(hLoadingBar,0); */
	/* CLOSE LOADING BAR */

    return true;
}

void SkinMe(HDC dc)
{
  BitBlt(dc, 0,0,ScreenWidth-4,ScreenHeight, dcSkin, 0,0, SRCCOPY);
}

void RegionMe()
{
  HRGN hRegion1 = CreateEllipticRgn(-59,-59,ScreenWidth+59,ScreenHeight+59);

  SetWindowRgn(hWnd, hRegion1, true);

  DeleteObject(hRegion1);

  DWORD dwStyle = GetWindowLong(hWnd, GWL_STYLE);
  dwStyle &= ~(WS_CAPTION|WS_SIZEBOX);
  SetWindowLong(hWnd, GWL_STYLE, dwStyle);

  // Repaint
  InvalidateRect(hWnd, NULL, TRUE);
  SetWindowPos(hWnd, NULL, 0,0,ScreenWidth-1,ScreenHeight-1, SWP_NOMOVE|SWP_NOZORDER);

  bRegioned = true;
}

void PlayAudio( MP3FileInfo& song )
{
	if(Status == 1) return;

	op.dwCallback = 0;
	op.lpstrDeviceType = (char*)MCI_ALL_DEVICE_ID;
	op.lpstrElementName = song.szFilename;
	op.lpstrAlias = 0;

	// Send command
	if ( mciSendCommand( 0, MCI_OPEN,
	MCI_OPEN_ELEMENT | MCI_WAIT,
	(DWORD)&op) == 0)
	{
		// Success on open
		pDevice = op.wDeviceID;
	}
	else
	{
		// Failed
	}

	MCI_PLAY_PARMS pp;
	pp.dwCallback = 0;

	if(Status == 0)
	{
		lastFile = song;
		char szCommandBuffer[512] = {char()}, szReturnBuffer[32] = {char()};

		::sprintf(szCommandBuffer, "open \"%s\" alias myfile", song.szFilename);

		DWORD dwDummy = ::mciSendString(szCommandBuffer, NULL, NULL, NULL);

 		if(!dwDummy)
 		{
			::sprintf(szCommandBuffer, "status myfile length", song.nLength);			
 			dwDummy = ::mciSendString(szCommandBuffer, szReturnBuffer, sizeof(szReturnBuffer), NULL);


			::mciSendString(szCommandBuffer, NULL, NULL, NULL);
 		}

		if(dwDummy) { }

		rSeconds = song.nLength;///60 + (((song.nLength % 60) * 60)/100);
		SendMessage(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, rSeconds));
		SendMessage(hProgress, PBM_SETPOS, 0, 0); 
		
		//float nSecond = song.nLength/60 + (((song.nLength % 60) * 60)/100);
		//seconds = (int)nSecond;
		//nSecond -= seconds;
		
		lockHour.Lock();
		seconds = (song.nLength % 60);
		hours = song.nLength/60/60;
		minutes = song.nLength/60;
		lockHour.Unlock();

		sprintf(Time, "%02d:%02d:%02d", hours, minutes, seconds);
		//InvalidateRect(hWnd, &rect, TRUE);
	}

		mciSendCommand(pDevice, MCI_PLAY, MCI_NOTIFY, (DWORD)&pp);
}

void PauseAudio()
{
	mciSendCommand( pDevice, MCI_PAUSE, 0, NULL );
}

void StopAudio()
{
	mciSendCommand( pDevice, MCI_STOP, 0, NULL );
	mciSendCommand( pDevice, MCI_CLOSE, 0, 0 );
	mciSendString( "close myfile", 0, 0, 0);
}

void StepAudio(int value)
{
	//PauseAudio();
	mciSendCommand( pDevice, MCI_STOP, 0, NULL );

	CString temp;
	temp.Format("play myfile from %d to %d notify", value * 1000, lastFile.nLength * 1000);
	mciSendString( temp, 0, 0, 0);
}
