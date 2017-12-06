/*----------------------------------------------------------------------
-- SOURCE FILE:
--
-- PROGRAM:
--
-- FUNCTIONS:
--				int WINAPI WinMain(HINSTANCE hInst
--								   , HINSTANCE hprevInstance,
--								   , LPSTR lspszCmdParam, int nCmdShow)
--				LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM)
--				boolean InitializeCommHandle(LPTSTR CommPort)
--				boolean SwitchCommPort(int commPort)
--				boolean ConfigureCommSettings(HWND hwnd)
--
--
--
-- DATE: November 29, 2017
--
-- DESIGNER: Jeremy Lee, Li-Yan Tong, Morgan Ariss, Luke Lee
--
-- PROGRAMMER: Luke Lee
--
-- NOTES:
-- The program reads user keystroke as well as serial port input,
-- sends user's character input to serial port, and displays incoming
-- serial port input on screen.
--
-- Before starting data exchange, user can configure serial port or
-- communication settings such as bit per second, data bits, parity,
-- stop bit, and flow control.
--
-- To start data exchange, user can select Connect menu, and start
-- typing on keyboard to send a character at a time to serial port.
-- At the same time, the program reads incoming serial port input and
-- displays it on the screen.
--
-- To access to port or communication settings again, user must
-- disconnect first. Reconnection or exiting the program is allowed
-- anytime during the operation.
----------------------------------------------------------------------*/
#pragma warning (disable: 4096)

#define STRICT

#include "Menu.h"
#include "Main.h"
#include "FileUploader.h"
#include "FileDownloader.h"
#include "PrintData.h"

using namespace protocoletariat;

/*----------------------------------------------------------------------
-- FUNCTION: WinMain
--
-- DATE: November 29, 2017
--
-- DESIGNER: Jeremy Lee
--
-- PROGRAMMER: Luke Lee
--
-- INTERFACE: int WINAPI WinMain(HINSTANCE hInst
--								 , HINSTANCE hprevInstance,
--								 , LPSTR lspszCmdParam, int nCmdShow)
--
-- RETURNS: int
--
-- NOTES:
-- Main function of this program. Its main role is creating Window and
-- initialize communciation Handler, as well as reading Windows Events
-- including user keystroke Events. Messages generated by Events captured
-- by this function are sent to WndProc
----------------------------------------------------------------------*/
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hprevInstance,
	LPSTR lspszCmdParam, int nCmdShow)
{
	MSG Msg;

	if (!InitializeWindows(hInst, nCmdShow))
	{
		return 0;
	}

	lpszCommPort = lpszDefaultCommPort;

	// initialize comm handle; if no serial port found, terminates the program
	//if (!InitializeCommHandle(lpszCommPort))
	//{
	//	return 0;
	//}

	logfile = new LogFile();
	fileUploadParam = new paramFileUploader();
	fileDownloadParam = new paramFileDownloader();
	printDataParam = new paramPrintData();

	StartEngine();
  
	while (GetMessage(&Msg, NULL, 0, 0))
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	return Msg.wParam;
}

/*----------------------------------------------------------------------
-- FUNCTION: InitializeWindows
--
-- DATE: November 29, 2017
--
-- DESIGNER: Luke Lee
--
-- PROGRAMMER: Luke Lee
--
-- INTERFACE: boolean InitializeWindows(HINSTANCE hInst, int nCmdShow)
--
-- RETURNS: boolean
--
-- NOTES:
-- This function initializes parameters for the wireless terminal Windows
-- and opens the windows.
----------------------------------------------------------------------*/
boolean protocoletariat::InitializeWindows(HINSTANCE hInst, int nCmdShow)
{
	// application Window values
	const int intWindowW = 500; // Window width
	const int intWindowH = 650; // Window height

	WNDCLASSEX Wcl;

	Wcl.cbSize = sizeof(WNDCLASSEX);
	Wcl.style = CS_HREDRAW | CS_VREDRAW;
	Wcl.hIcon = LoadIcon(NULL, IDI_APPLICATION); // large icon 
	Wcl.hIconSm = NULL; // use small version of large icon
	Wcl.hCursor = LoadCursor(NULL, IDC_ARROW); // cursor style

	Wcl.lpfnWndProc = WndProc;
	Wcl.hInstance = hInst;
	Wcl.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	Wcl.lpszClassName = tchrProgramName;

	Wcl.lpszMenuName = TEXT("MYMENU"); // The menu Class
	Wcl.cbClsExtra = 0; // no extra memory needed
	Wcl.cbWndExtra = 0;

	if (!RegisterClassEx(&Wcl))
	{
		MessageBox(NULL, "Wireless Terminal couldn't start!", NULL, MB_OK | MB_ICONSTOP);
		return false;
	}

	hwnd = CreateWindow(tchrProgramName, tchrProgramName
		, WS_OVERLAPPEDWINDOW, 10, 10, intWindowW, intWindowH, NULL
		, NULL, hInst, NULL);
	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	return true;
}

/*----------------------------------------------------------------------
-- FUNCTION: WndProc
--
-- DATE: November 29, 2017
--
-- DESIGNER: Jeremy Lee
--
-- PROGRAMMER: Luke Lee
--
-- INTERFACE: LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM)
--
-- RETURNS: LRESULT
--
-- NOTES:
-- This function receives Messages from WinMain and determines behavior
-- in switch statements. Selecting on menu items defined in Menu.h and Menu.rc
-- is detected in this function, and it behaves accordingly.
----------------------------------------------------------------------*/
LRESULT CALLBACK protocoletariat::WndProc(HWND hwnd, UINT Message,
	WPARAM wParam, LPARAM lParam)
{
	char bufferWrite[2] = "";
	DWORD dwWritten;
	HDC hdc;
	PAINTSTRUCT paintstruct;
	TEXTMETRIC tm;

	switch (Message)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam)) // menu
		{
		case IDM_UPLOAD: // Start upload file thread

			OPENFILENAME ofn;       // common dialog box structure
			char szFile[300];       // buffer for file name

			// Initialize OPENFILENAME
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = hwnd;
			ofn.lpstrFile = szFile;

			ofn.lpstrFile[0] = '\0';
			ofn.nMaxFile = sizeof(szFile);
			ofn.lpstrFilter = "All\0*.*\0Text\0*.TXT\0";
			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = NULL;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

			// Display the Open dialog box. 
			if (GetOpenFileName(&ofn) == TRUE)
			{
				fileUploadParam->filePath = ofn.lpstrFile;
				fileUploadParam->uploadQueue = &uploadQ;
				uploadThrd = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) FileUploader::LoadTextFile, fileUploadParam, 0, &uploadThrdID);
			}
			else
			{
				MessageBox(hwnd
					, TEXT("Unable to open specified file")
					, TEXT("Access Denied"), MB_ICONHAND | MB_OK);
			}
			break;

		case IDM_CONFIG: // Settings > Configure
			if (!bCommOn)
				ConfigureCommSettings(hwnd);
			else
			{
				MessageBox(hwnd
					, TEXT("Configuration is disabled while connected")
					, TEXT("Access Denied"), MB_ICONHAND | MB_OK);
			}
			break;

		case IDM_COM1:
			if (!bCommOn)
				SwitchCommPort(1);
			else
			{
				MessageBox(hwnd
					, TEXT("Cannot switch port while connected")
					, TEXT("Access Denied"), MB_ICONHAND | MB_OK);
			}
			break;

		case IDM_COM2:
			if (!bCommOn)
				SwitchCommPort(2);
			else
			{
				MessageBox(hwnd
					, TEXT("Cannot switch port while connected")
					, TEXT("Access Denied"), MB_ICONHAND | MB_OK);
			}
			break;

		case IDM_COM3:
			if (!bCommOn)
				SwitchCommPort(3);
			else
			{
				MessageBox(hwnd
					, TEXT("Cannot switch port while connected")
					, TEXT("Access Denied"), MB_ICONHAND | MB_OK);
			}
			break;

		case IDM_COM4:
			if (!bCommOn)
				SwitchCommPort(4);
			else
			{
				MessageBox(hwnd
					, TEXT("Cannot switch port while connected")
					, TEXT("Access Denied"), MB_ICONHAND | MB_OK);
			}
			break;

		case IDM_ABOUT: // Open About dialog
			//StopCommunication();
			break;

		case IDM_HELP: // Open user manual
			//StopCommunication();
			break;

		case IDM_EXIT: // Exit program
			if (IDOK == MessageBox(hwnd, "OK to close window?", "Exit", MB_ICONQUESTION | MB_OKCANCEL))
			{
				CleanUp();
			}
			break;
		}
		break;

	case WM_CHAR: // process keystroke
		if (wParam == VK_ESCAPE)
		{
			if (IDOK == MessageBox(hwnd, "OK to close window?", "Exit", MB_ICONQUESTION | MB_OKCANCEL))
			{
				CleanUp();
			}
			break;
		}
		else if (wParam == RVI_KEY)
		{
			MessageBox(hwnd, "RVI key pressed", "Title", MB_OKCANCEL);
			break;
		}
		break;

	case WM_PAINT: // process a repaint message
		hdc = BeginPaint(hwnd, &paintstruct); // Acquire DC
		GetTextMetrics(hdc, &tm); // get text metrics
		ReleaseDC(hwnd, hdc); // release device context
		EndPaint(hwnd, &paintstruct); // Release DC

		break;

	case WM_DESTROY: // terminate program
		if (IDOK == MessageBox(hwnd, "OK to close window?", "Exit", MB_ICONQUESTION | MB_OKCANCEL))
		{
			CleanUp();
		}
		break;

	default:
		return DefWindowProc(hwnd, Message, wParam, lParam);
	}

	return 0;
}

/*----------------------------------------------------------------------
-- FUNCTION: InitializeCommHandle
--
-- DATE: October 4, 2017
--
-- DESIGNER: Jeremy Lee
--
-- PROGRAMMER: Jeremy Lee
--
-- INTERFACE: boolean InitializeCommHandle(LPTSTR CommPort)
--
-- RETURNS: boolean
--
-- NOTES:
-- This function creates communication Handle and applies the
-- configuration settings to it.
----------------------------------------------------------------------*/
boolean protocoletariat::InitializeCommHandle(LPTSTR CommPort)
{
	// create communcation handle
	hComm = CreateFile(CommPort, GENERIC_READ | GENERIC_WRITE, 0,
		NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (hComm == INVALID_HANDLE_VALUE) // failed to create handle
	{
		MessageBox(NULL, "No Serial COM port found.", NULL, MB_OK | MB_ICONSTOP);
		// TODO: release hComm memory
		return false;
	}

	ccfg.dwSize = sizeof(COMMCONFIG);
	ccfg.wVersion = 0x100;
	if (!GetCommConfig(hComm, &ccfg, &ccfg.dwSize))
	{
		MessageBox(NULL, "Error getting the COM port configuration dialog", TEXT("Error"), MB_OK);
		return false;
	}
	if (!SetCommState(hComm, &ccfg.dcb))
	{
		MessageBox(NULL, "Error setting the COM port configuration", TEXT("Error"), MB_OK);
		return false;
	}

	return true;
}

/*----------------------------------------------------------------------
-- FUNCTION: SwitchCommPort
--
-- DATE: October 4, 2017
--
-- DESIGNER: Jeremy Lee
--
-- PROGRAMMER: Jeremy Lee
--
-- INTERFACE: boolean SwitchCommPort(int commPort)
--
-- RETURNS: boolean
--
-- NOTES:
-- This function is called by user Menu click in WndProc, and sets the
-- target COM port based on the user selection. There are currently 4
-- COM ports available.
----------------------------------------------------------------------*/
boolean protocoletariat::SwitchCommPort(int commPort)
{
	switch (commPort)
	{
	case 1:
		lpszCommPort = TEXT("COM1");
		break;
	case 2:
		lpszCommPort = TEXT("COM2");
		break;
	case 3:
		lpszCommPort = TEXT("COM3");
		break;
	case 4:
		lpszCommPort = TEXT("COM4");
		break;
	default:
		return false;
	}

	PurgeComm(hComm, PURGE_RXCLEAR); // clean out pending bytes
	PurgeComm(hComm, PURGE_TXCLEAR); // clean out pending bytes
	CloseHandle(hComm);
	if (!InitializeCommHandle(lpszCommPort))
		return false;

	return true;
}

/*----------------------------------------------------------------------
-- FUNCTION: ConfigureCommSettings
--
-- DATE: October 4, 2017
--
-- DESIGNER: Jeremy Lee
--
-- PROGRAMMER: Jeremy Lee
--
-- INTERFACE: boolean ConfigureCommSettings(HWND hwnd)
--
-- RETURNS: boolean
--
-- NOTES:
-- This function is called by user Menu click that is processed in
-- WndProc. Once called, it displays a separate Window containing
-- communication settings. On that Window, user can configure values for
-- communication properties, and apply it for the next connection.
----------------------------------------------------------------------*/
boolean protocoletariat::ConfigureCommSettings(HWND hwnd)
{
	ccfg.dwSize = sizeof(COMMCONFIG);
	ccfg.wVersion = 0x100;
	if (!GetCommConfig(hComm, &ccfg, &ccfg.dwSize))
	{
		MessageBox(NULL, "Error getting the COM port configuration dialog", TEXT("Error"), MB_OK);
		return false;
	}
	CommConfigDialog(lpszCommPort, hwnd, &ccfg);
	if (!SetCommState(hComm, &ccfg.dcb))
	{
		MessageBox(NULL, "Error setting the COM port configuration", TEXT("Error"), MB_OK);
		return false;
	}

	return true;
}

/*----------------------------------------------------------------------
-- FUNCTION: StartEngine
--
-- DATE: December 4, 2017
--
-- DESIGNER: Luke Lee
--
-- PROGRAMMER: Luke Lee
--
-- INTERFACE: boolean ConfigureCommSettings(HWND hwnd)
--
-- RETURNS: boolean
--
-- NOTES:
-- This function is responsible for initializing and starting the download
-- thread, print data thread, and the main protocol engine thread. It
-- initializes the required parameters in custom structs and passes them
-- to the thread process in each of their respective classes.
----------------------------------------------------------------------*/
void protocoletariat::StartEngine()
{

	// initialize download (read) thread
	olRead = { 0 };
	fileDownloadParam->downloadQueue = &downloadQ;
	fileDownloadParam->olRead = olRead;
	fileDownloadParam->dwThreadExit = readThreadExit;
	fileDownloadParam->handle = &hComm;
	downloadThrd = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)FileDownloader::ReadSerialPort, fileDownloadParam, 0, &downloadThrdID);
	
	// initialize print data thread
	printDataParam->printQueue = &dataToPrintQ;
	printDataParam->hwnd = &hwnd;
	printDataParam->hComm = &hComm;
	printDataParam->X = &X;
	printDataParam->Y = &Y;
	printDataParam->logfile = logfile;
	printThrd = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)PrintData::PrintReceivedData, printDataParam, 0, &printThrdID);

	// initialize main protocol engine thread
	//olWrite = { 0 };
	//protocolParam->uploadQueue = &uploadQ;
	//protocolParam->downloadQueue = &downloadQ;
	//protocolParam->printQueue = &dataToPrintQ;
	//protocolParam->olWrite = olWrite;
	//protocolParam->dwThreadExit = writeThreadExit;
	//protocolParam->handle = hComm;
	//protocolParam->logfile = logfile;
	//protocolThrd = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ProtocolThread, protocolParam, 0, &protocolThrdID);

}

void protocoletariat::ClearQueue(std::queue<char*> &q)
{
	while (!q.empty())
	{
		q.pop();
	}
}

void protocoletariat::CleanUp()
{
	CloseHandle(hComm);

	ClearQueue(uploadQ);
	ClearQueue(downloadQ);
	ClearQueue(dataToPrintQ);

	CloseHandle(uploadThrd);
	CloseHandle(downloadThrd);
	CloseHandle(printThrd);
	CloseHandle(protocolThrd);

	delete logfile;
	delete fileUploadParam;

	PostQuitMessage(0);
}
