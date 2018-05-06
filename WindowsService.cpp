#include "stdafx.h"

#include "WindowsService.h"

Windows::Service* Windows::Service::pService = NULL;
SERVICE_STATUS Windows::Service::g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE Windows::Service::g_StatusHandle = NULL;
HANDLE Windows::Service::g_ServiceStopEvent = INVALID_HANDLE_VALUE;

Windows::Service::Service(
	const wchar_t* serviceName,
	const wchar_t* displayName,
	DWORD serviceStartType,
	const wchar_t* serviceDependencies,
	const wchar_t* serviceAccount,
	const wchar_t* servicePassword,
	FunctionServe serve)
{
	this->serviceName = serviceName;
	this->displayName = displayName;
	this->serviceStartType = serviceStartType;
	this->serviceDependencies = serviceDependencies;
	this->serviceAccount = serviceAccount;
	this->servicePassword = servicePassword;
	this->serve = serve;
}

Windows::Service::~Service()
{

}

/**
install service to Windows Service
*/

int Windows::Service::Install()
{
	wchar_t szPath[MAX_PATH];
	SC_HANDLE schSCManager = NULL;
	SC_HANDLE schService = NULL;

	if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath)) == 0)
	{
		wprintf(L"GetModuleFileName Error: 0x%08lx\n", GetLastError());
		CleanUp(schSCManager, schService);
		return EXIT_FAILURE;
	}

	// Open the local default service control manager database
	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT |
		SC_MANAGER_CREATE_SERVICE);
	if (schSCManager == NULL)
	{
		wprintf(L"OpenSCManager Error: 0x%08lx\n", GetLastError());
		CleanUp(schSCManager, schService);
		return EXIT_FAILURE;
	}

	// Install the service into SCM by calling CreateService
	int status = EXIT_FAILURE;
	schService = CreateService(
		schSCManager,                   // SCManager database
		serviceName,                 // Name of service
		displayName,                 // Name to display
		SERVICE_QUERY_STATUS,           // Desired access
		SERVICE_WIN32_OWN_PROCESS,      // Service type
		serviceStartType,                    // Service start type
		SERVICE_ERROR_NORMAL,		     // Error control type
		szPath,                         // Service's binary
		NULL,                           // No load ordering group
		NULL,                           // No tag identifier
		serviceDependencies,                // Dependencies
		serviceAccount,                     // Service running account
		servicePassword                     // Password of the account
	);
	if (schService == NULL)
	{
		wprintf(L"CreateService Error 0x%08lx\n", GetLastError());
		status = EXIT_FAILURE;
	}
	else
	{
		wprintf(L"%s Installed.\n", serviceName);
		status = EXIT_SUCCESS;
	}

	CleanUp(schSCManager, schService);
	return status;
}

/**
uninstall service from Windows Service
*/

int Windows::Service::Uninstall()
{
	SC_HANDLE schSCManager = NULL;
	SC_HANDLE schService = NULL;
	SERVICE_STATUS ssSvcStatus = {};

	// Open the local default service control manager database
	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (schSCManager == NULL)
	{
		wprintf(L"OpenSCManager Error: 0x%08lx\n", GetLastError());
		CleanUp(schSCManager, schService);
		return EXIT_FAILURE;
	}

	// Open the service with delete, stop, and query status permissions
	schService = OpenService(schSCManager, serviceName, SERVICE_STOP |
		SERVICE_QUERY_STATUS | DELETE);
	if (schService == NULL)
	{
		wprintf(L"OpenService Error: 0x%08lx\n", GetLastError());
		CleanUp(schSCManager, schService);
		return EXIT_FAILURE;
	}

	// Try to stop the service
	if (ControlService(schService, SERVICE_CONTROL_STOP, &ssSvcStatus))
	{
		wprintf(L"Stopping %s.", serviceName);
		Sleep(1000);

		while (QueryServiceStatus(schService, &ssSvcStatus))
		{
			if (ssSvcStatus.dwCurrentState == SERVICE_STOP_PENDING)
			{
				wprintf(L".");
				Sleep(1000);
			}
			else break;
		}

		if (ssSvcStatus.dwCurrentState == SERVICE_STOPPED)
		{
			wprintf(L"\n%s Stopped.\n", serviceName);
		}
		else
		{
			wprintf(L"\n%s cannot stop.\n", serviceName);
			CleanUp(schSCManager, schService);
			return EXIT_FAILURE;
		}
	}

	// Now remove the service by calling DeleteService.
	int status = EXIT_SUCCESS;
	if (!DeleteService(schService))
	{
		wprintf(L"DeleteService Error: 0x%08lx\n", GetLastError());
		status = EXIT_FAILURE;
	}
	else
	{
		wprintf(L"%s Removed.\n", serviceName);
		status = EXIT_SUCCESS;
	}
	CleanUp(schSCManager, schService);
	return status;
}


/**
cleanup resources
*/

void Windows::Service::CleanUp(SC_HANDLE schSCManager, SC_HANDLE schService)
{
	if (schSCManager)
	{
		CloseServiceHandle(schSCManager);
		schSCManager = NULL;
	}
	if (schService)
	{
		CloseServiceHandle(schService);
		schService = NULL;
	}
}


int Windows::Service::Main(
	const wchar_t* serviceName,
	const wchar_t* displayName,
	DWORD serviceStartType,
	const wchar_t* serviceDependencies,
	const wchar_t* serviceAccount,
	const wchar_t* servicePassword,
	FunctionServe serve,
	int argc,
	wchar_t *argv[])
{
	if (!pService)
	{
		pService = new Service(
			serviceName,
			displayName,
			SERVICE_DEMAND_START,
			serviceDependencies,
			serviceAccount,
			servicePassword,
			serve);
	}

	if (pService)
	{
		int status = 0;
		if ((argc > 1) && ((*argv[1] == L'-' || (*argv[1] == L'/'))))
		{
			if (_wcsicmp(L"install", argv[1] + 1) == 0)
			{

				// Install the service when the command is 
				// "-install" or "/install".
				status = pService->Install();
			}
			else if (_wcsicmp(L"remove", argv[1] + 1) == 0)
			{
				// Uninstall the service when the command is 
				// "-remove" or "/remove".
				status = pService->Uninstall();
			}
			return status;
		}
		else
		{
			SERVICE_TABLE_ENTRY ServiceTable[] =
			{
				{ (LPWSTR)pService->serviceName, (LPSERVICE_MAIN_FUNCTION)ServiceMain },
			{ NULL, NULL }
			};

			if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
			{
				return GetLastError();
			}
			return EXIT_SUCCESS;
		}
	}
	return EXIT_FAILURE;
}

VOID WINAPI Windows::Service::ServiceMain(DWORD argc, LPTSTR *argv)
{
	DWORD Status = E_FAIL;
	g_StatusHandle = RegisterServiceCtrlHandler(pService->serviceName, (LPHANDLER_FUNCTION)ServiceCtrlHandler);
	if (g_StatusHandle == NULL)
	{
		return;
	}
	else
	{
		// Tell the service controller we are starting
		ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
		g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
		g_ServiceStatus.dwControlsAccepted = 0;
		g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
		g_ServiceStatus.dwWin32ExitCode = 0;
		g_ServiceStatus.dwServiceSpecificExitCode = 0;
		g_ServiceStatus.dwCheckPoint = 0;

		if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
		{
			return;
		}


		// Create stop event to wait on later.
		g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (g_ServiceStopEvent == NULL)
		{
			g_ServiceStatus.dwControlsAccepted = 0;
			g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
			g_ServiceStatus.dwWin32ExitCode = GetLastError();
			g_ServiceStatus.dwCheckPoint = 1;

			if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
			{
				return;
			}
		}
		else
		{
			// Tell the service controller we are started
			g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
			g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
			g_ServiceStatus.dwWin32ExitCode = 0;
			g_ServiceStatus.dwCheckPoint = 0;

			if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
			{
				return;
			}

			// Start the thread that will perform the main task of the service
			HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ServiceWorkerThread, NULL, 0, NULL);

			// Wait until our worker thread exits effectively signaling that the service needs to stop
			WaitForSingleObject(hThread, INFINITE);
			/*
			* Perform any cleanup tasks
			*/

			CloseHandle(g_ServiceStopEvent);

			g_ServiceStatus.dwControlsAccepted = 0;
			g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
			g_ServiceStatus.dwWin32ExitCode = 0;
			g_ServiceStatus.dwCheckPoint = 3;

			if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
			{
				return;
			}
		}
		return;
	}
}
VOID WINAPI Windows::Service::ServiceCtrlHandler(DWORD CtrlCode)
{
	switch (CtrlCode)
	{
	case SERVICE_CONTROL_STOP:
		if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
			break;

		/*
		* Perform tasks neccesary to stop the service here
		*/

		pService->g_ServiceStatus.dwControlsAccepted = 0;
		pService->g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		pService->g_ServiceStatus.dwWin32ExitCode = 0;
		pService->g_ServiceStatus.dwCheckPoint = 4;

		if (SetServiceStatus(pService->g_StatusHandle, &pService->g_ServiceStatus) == FALSE)
		{
			return;
		}

		// This will signal the worker thread to start shutting down
		SetEvent(pService->g_ServiceStopEvent);

		break;

	default:
		break;
	}
}
DWORD WINAPI Windows::Service::ServiceWorkerThread(LPVOID lpParam)
{
	//  Periodically check if the service has been requested to stop
	while (WaitForSingleObject(pService->g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
	{
		/*
		* Perform main service function here
		*/

		if (pService)
		{
			pService->serve();
		}

		//  Simulate some work by sleeping
		Sleep(3000);
	}
	return ERROR_SUCCESS;
}

