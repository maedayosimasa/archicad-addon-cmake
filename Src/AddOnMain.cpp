#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma warning(disable : 4819)
#include "APIEnvir.h"
#include "ACAPinc.h"

#include "ResourceIds.hpp"
#include "DGModule.hpp"
#include "Location.hpp"

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <vector>
#include <string>

#pragma comment(lib, "ws2_32.lib")

static const GSResID AddOnInfoID			= ID_ADDON_INFO;
	static const Int32 AddOnNameID			= 1;
	static const Int32 AddOnDescriptionID	= 2;

static const short AddOnMenuID				= ID_ADDON_MENU;
	static const Int32 AddOnCommandID		= 1;

static HANDLE serverProcessHandle = NULL;

// --- Update Command Implementation ---
static GSErrCode UpdateElements (const std::string& jsonCommand)
{
	return ACAPI_CallUndoableCommand ("Update Elements from Python", [&] () -> GSErrCode {
		GS::String msg = GS::String::SPrintf ("Applying updates from Python (Data size: %d bytes)", (int) jsonCommand.length ());
		ACAPI_WriteReport (msg.ToCStr (), false);
		return NoError;
	});
}

// --- Background Listener Thread ---
void StartCommandListener ()
{
	std::thread ([] () {
		SOCKET listenSocket = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (listenSocket == INVALID_SOCKET) return;

		sockaddr_in service;
		service.sin_family = AF_INET;
		service.sin_addr.s_addr = inet_addr ("127.0.0.1");
		service.sin_port = htons (5001);

		if (bind (listenSocket, (SOCKADDR*) &service, sizeof (service)) == SOCKET_ERROR) {
			closesocket (listenSocket);
			return;
		}

		listen (listenSocket, 1);

		while (true) {
			SOCKET acceptSocket = accept (listenSocket, NULL, NULL);
			if (acceptSocket != INVALID_SOCKET) {
				char buffer[4096];
				int bytesReceived = recv (acceptSocket, buffer, 4096, 0);
				if (bytesReceived > 0) {
					std::string cmd (buffer, bytesReceived);
					UpdateElements (cmd);
				}
				closesocket (acceptSocket);
			}
		}
	}).detach ();
}

static void SendDataToPython (const GS::String& jsonStr)
{
	SOCKET connectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL, hints;

	ZeroMemory (&hints, sizeof (hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	if (getaddrinfo ("127.0.0.1", "5000", &hints, &result) != 0) {
		ACAPI_WriteReport ("DNS Error: Failed to resolve localhost.", false);
		return;
	}

	connectSocket = socket (result->ai_family, result->ai_socktype, result->ai_protocol);
	if (connectSocket == INVALID_SOCKET) {
		freeaddrinfo (result);
		ACAPI_WriteReport ("Socket Error: Failed to create socket.", false);
		return;
	}

	if (connect (connectSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
		closesocket (connectSocket);
		connectSocket = INVALID_SOCKET;
	}

	freeaddrinfo (result);

	if (connectSocket == INVALID_SOCKET) {
		ACAPI_WriteReport ("Connection Error: Python server is not responding on port 5000.", false);
		return;
	}

	send (connectSocket, jsonStr.ToCStr (), (int)jsonStr.GetLength (), 0);
	closesocket (connectSocket);
}

static void ExportElementsToPython ()
{
	GS::Array<API_Guid> elemGuids;
	GSErrCode err = ACAPI_Element_GetElemList (API_ZombieElemID, &elemGuids);
	if (err != NoError) {
		ACAPI_WriteReport ("Error: Failed to get element list.", false);
		return;
	}

	GS::String jsonStr = "{\"elements\": [";
	bool first = true;

	for (const API_Guid& guid : elemGuids) {
		API_Element element = {};
		element.header.guid = guid;
		if (ACAPI_Element_GetHeader (&element.header) == NoError) {
			if (!first) jsonStr += ",";
			
			GS::UniString typeName;
			ACAPI_Element_GetElemTypeName (element.header.type, typeName);

			jsonStr += "{";
			jsonStr += "\"guid\": \"" + GS::String(APIGuidToString (guid).ToCStr()) + "\",";
			jsonStr += "\"type\": \"" + GS::String(typeName.ToCStr()) + "\",";
			jsonStr += "\"floor\": " + GS::String::SPrintf("%d", element.header.floorInd);
			jsonStr += "}";
			
			first = false;
		}
	}
	jsonStr += "]}";

	SendDataToPython (jsonStr);
}

class ExampleDialog :	public DG::ModalDialog,
						public DG::PanelObserver,
						public DG::ButtonItemObserver,
						public DG::CompoundItemObserver
{
public:
	enum DialogResourceIds
	{
		ExampleDialogResourceId = ID_ADDON_DLG,
		OKButtonId = 1,
		CancelButtonId = 2,
		SeparatorId = 3,
		GetConfigButtonId = 4
	};

	ExampleDialog () :
		DG::ModalDialog (ACAPI_GetOwnResModule (), ExampleDialogResourceId, ACAPI_GetOwnResModule ()),
		okButton (GetReference (), OKButtonId),
		cancelButton (GetReference (), CancelButtonId),
		separator (GetReference (), SeparatorId),
		getConfigButton (GetReference (), GetConfigButtonId)
	{
		GS::UniString text;
		if (RSGetIndString (&text, ExampleDialogResourceId, 1, ACAPI_GetOwnResModule ())) SetTitle (text);
		if (RSGetIndString (&text, ExampleDialogResourceId, 2, ACAPI_GetOwnResModule ())) okButton.SetText (text);
		if (RSGetIndString (&text, ExampleDialogResourceId, 3, ACAPI_GetOwnResModule ())) cancelButton.SetText (text);
		if (RSGetIndString (&text, ExampleDialogResourceId, 5, ACAPI_GetOwnResModule ())) getConfigButton.SetText (text);

		AttachToAllItems (*this);
		Attach (*this);
	}

	~ExampleDialog ()
	{
		Detach (*this);
		DetachFromAllItems (*this);
	}

	virtual void ButtonClicked (const DG::ButtonClickEvent& ev) override
	{
		if (ev.GetSource () == &okButton) {
			PostCloseRequest (DG::ModalDialog::Accept);
		} else if (ev.GetSource () == &cancelButton) {
			PostCloseRequest (DG::ModalDialog::Cancel);
		} else if (ev.GetSource () == &getConfigButton) {
			ExportElementsToPython ();
			ACAPI_WriteReport ("Data exported to PyQt.", false);
		}
	}

	DG::Button		okButton;
	DG::Button		cancelButton;
	DG::Separator	separator;
	DG::Button		getConfigButton;
};

static GSErrCode MenuCommandHandler (const API_MenuParams *menuParams)
{
	switch (menuParams->menuItemRef.menuResID) {
		case AddOnMenuID:
			switch (menuParams->menuItemRef.itemIndex) {
				case AddOnCommandID:
					{
						ExampleDialog dialog;
						dialog.Invoke ();
					}
					break;
			}
			break;
	}
	return NoError;
}

API_AddonType CheckEnvironment (API_EnvirParams* envir)
{
	RSGetIndString (&envir->addOnInfo.name, AddOnInfoID, AddOnNameID, ACAPI_GetOwnResModule ());
	RSGetIndString (&envir->addOnInfo.description, AddOnInfoID, AddOnDescriptionID, ACAPI_GetOwnResModule ());

	return APIAddon_Normal;
}

GSErrCode RegisterInterface (void)
{
#ifdef ServerMainVers_2700
	return ACAPI_MenuItem_RegisterMenu (AddOnMenuID, 0, MenuCode_Tools, MenuFlag_Default);
#else
	return ACAPI_Register_Menu (AddOnMenuID, 0, MenuCode_Tools, MenuFlag_Default);
#endif
}

GSErrCode Initialize (void)
{
	WSADATA wsaData;
	WSAStartup (MAKEWORD (2, 2), &wsaData);
	
	StartCommandListener ();

	IO::Location addOnLoc;
	GSErrCode err = ACAPI_GetOwnLocation (&addOnLoc);
	if (err == NoError) {
		IO::Location projectRoot = addOnLoc;
		projectRoot.DeleteLastLocalName ();
		projectRoot.DeleteLastLocalName ();
		projectRoot.DeleteLastLocalName ();
		
		GS::UniString projectPath;
		projectRoot.ToPath (&projectPath);

		std::wstring pythonPath = L"python";
		std::wstring scriptPath = std::wstring(projectPath.ToUStr().Get()) + L"\\server.py";
		std::wstring commandLine = pythonPath + L" \"" + scriptPath + L"\"";
		
		ACAPI_WriteReport ("Starting Python server...", false);

		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory (&si, sizeof (si));
		si.cb = sizeof (si);
		ZeroMemory (&pi, sizeof (pi));

		if (CreateProcess (NULL, (LPWSTR)commandLine.c_str (), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
			serverProcessHandle = pi.hProcess;
			CloseHandle (pi.hThread);
			ACAPI_WriteReport ("Python server process created.", false);
		} else {
			ACAPI_WriteReport ("Failed to start Python server.", false);
		}
	}

#ifdef ServerMainVers_2700
	return ACAPI_MenuItem_InstallMenuHandler (AddOnMenuID, MenuCommandHandler);
#else
	return ACAPI_Install_MenuHandler (AddOnMenuID, MenuCommandHandler);
#endif
}

GSErrCode FreeData (void)
{
	if (serverProcessHandle != NULL) {
		TerminateProcess (serverProcessHandle, 0);
		CloseHandle (serverProcessHandle);
		serverProcessHandle = NULL;
	}

	WSACleanup ();
	return NoError;
}
