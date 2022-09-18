#include <iostream>
#include <process.h>
#include <windows.h>

unsigned __stdcall threadWork(void* data) {
	std::cout << "That was printed from thread number " << GetCurrentThreadId() << std::endl
		<< "It`s priority is " << GetThreadPriority(GetCurrentThread()) << std::endl;
	return 0;
}

int main(void) {
	std::cout << "Creating process with CreateProcess() function." << std::endl
		<< "Trying to open notepad with changed priority..." << std::endl;

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	std::wstring process = L"C:\\Windows\\system32\\notepad.exe";
	if (!CreateProcess(
		LPCWSTR(process.c_str()), // Name of the process
		NULL, //CL parameters
		NULL, //Process not inheritable
		NULL, //Thread not inheritable
		FALSE, //Flag of handle inheritance
		ABOVE_NORMAL_PRIORITY_CLASS, //Changing priority
		NULL, // Environment block
		NULL, //Directory info
		&si,
		&pi)
	) {
		std::cout << "Creating process failed. ERROR: " << GetLastError() << std::endl;
	}

	WaitForSingleObject(pi.hProcess, INFINITE); // Waiting for process to exit

	//Closing handles
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	std::cout << "Process was closed." << std::endl << std::endl;
	std::cout << "Showing of threads creation." << std::endl << std::endl;

	//Creating 2 threads and setting priority
	HANDLE firstThread;
	HANDLE secondThread;

	firstThread = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, &threadWork, NULL, 0, NULL));

	secondThread = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, &threadWork, NULL, 0, NULL));

	HANDLE handles[2] = { firstThread, secondThread };

	SetThreadPriority(secondThread, THREAD_PRIORITY_HIGHEST);

	WaitForMultipleObjects(2, handles, true, INFINITE);
	CloseHandle(firstThread);
	CloseHandle(secondThread);

	return 0;
}