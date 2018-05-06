# WindowsService
Class manage Windows Service - Install, Uninstall, Start, Stop action.
I modify and combine from 2 source code
1. https://code.msdn.microsoft.com/windowsapps/CppWindowsService-cacf4948
2. https://www.codeproject.com/Articles/499465/Simple-Windows-Service-in-Cplusplus

Tested in Visual Studio 2017, 64bit platform and Unicode Character Set.

# Getting Started
1. Create new project name is "SampleService", project type is "Windows Console Application"
2. add WindowsService.cpp and WindowsService.h in your project
3. Write script below
```
#include "stdafx.h"
#include "WindowsService.h"

void serve()
{
   //your logic application
}

int wmain(int argc, wchar_t *argv[])
{
	const wchar_t* serviceName = _T("SampleService");
	const wchar_t* displayName = _T("Sample Service");
	const wchar_t* serviceDependencies = _T("");
	const wchar_t* serviceAccount = _T("NT AUTHORITY\\LocalService");
	const wchar_t* servicePassword = NULL;
	int status = EXIT_FAILURE;

	//run the main service
	return Windows::Service::Main(
		serviceName,
		displayName,
		SERVICE_DEMAND_START,
		serviceDependencies,
		serviceAccount,
		servicePassword,
		serve,
		argc,
		argv);
}

```
4. Change project to 64bit platform.
5. Build project. Output at /x64/SampleService.exe
4. Open Console Prompt. (run as administrator)
5. SampleService.exe -install
6. SampleService.exe - remove
7. net start SampleService
8. net stop SampleService
