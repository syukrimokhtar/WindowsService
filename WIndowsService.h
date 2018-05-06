#include "stdafx.h"

#include <iostream>
#include <string>
#include <Windows.h>
#include <functional>

using FunctionServe = std::function<void()>;

namespace Windows
{

	/**
	convert char* to wchar_t*
	*/
	inline wchar_t* Char2WCharT(char* s)
	{
		if (s == NULL)
		{
			return NULL;
		}
		const size_t cSize = strlen(s) + 1;
		wchar_t* wc = new wchar_t[cSize];
		size_t tmp = 0;
		mbstowcs_s(&tmp, wc, cSize, s, cSize);
		return wc;
	}


	inline LPWSTR WCharT2LPWSTR(const wchar_t* s)
	{
		return (LPWSTR)s;
	}




	/**
	Service
	*/
	class Service
	{
	private:
		const wchar_t* serviceName;
		const wchar_t* displayName;
		DWORD serviceStartType;
		const wchar_t* serviceDependencies;
		const wchar_t* serviceAccount;
		const wchar_t* servicePassword;
		FunctionServe serve;

		void CleanUp(SC_HANDLE schSCManager, SC_HANDLE schService);

		static Service* pService;
		static SERVICE_STATUS g_ServiceStatus;
		static SERVICE_STATUS_HANDLE g_StatusHandle;
		static HANDLE g_ServiceStopEvent;

		static VOID WINAPI ServiceMain(DWORD argc, LPTSTR *argv);
		static VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode);
		static DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);



	public:
		
		Service(
			const wchar_t* serviceName,
			const wchar_t* displayName,
			DWORD serviceStartType,
			const wchar_t* serviceDependencies,
			const wchar_t* serviceAccount,
			const wchar_t* servicePassword,
			FunctionServe serve);
		~Service();
		int Install();
		int Uninstall();

		static int Main(
			const wchar_t* serviceName,
			const wchar_t* displayName,
			DWORD serviceStartType,
			const wchar_t* serviceDependencies,
			const wchar_t* serviceAccount,
			const wchar_t* servicePassword,
			FunctionServe serve,
			int argc,
			wchar_t *argv[]);
	};
}
