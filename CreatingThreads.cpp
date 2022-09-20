#include <iostream>
#include <process.h>
#include <windows.h>
#include <mutex>
#include <vector>

struct ThreadParams {
	ThreadParams(std::mutex& mut, int number) : mutex(mut), num(number) {};
	int num;
	std::mutex &mutex;
};

unsigned __stdcall threadWork(void* data) {
	std::cout << "That was printed from thread number " << GetCurrentThreadId() << std::endl
		<< "It`s priority is " << GetThreadPriority(GetCurrentThread()) << std::endl;
	return 0;
}

unsigned __stdcall mutexWork(void* data) {
	ThreadParams* params = static_cast<ThreadParams*>(data);
	int number = params->num;
	std::scoped_lock lock(params->mutex);
	std::cout << std::endl << "My number is " << number << std::endl;
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

	firstThread = reinterpret_cast<HANDLE>(_beginthreadex(
		NULL, //Not inheritable
		0, // Size of stack
		&threadWork, //Function that the thread will do
		NULL, //Arguments for the function
		0, //Initial state flag 
		NULL //Pointer to a variable for thread address if needed
	));

	secondThread = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, &threadWork, NULL, 0, NULL));

	HANDLE handles[] = { firstThread, secondThread };

	SetThreadPriority(secondThread, THREAD_PRIORITY_HIGHEST);

	WaitForMultipleObjects(2, handles, true, INFINITE);
	CloseHandle(firstThread);
	CloseHandle(secondThread);

	std::cout << std::endl << std::endl << "Maybe created threads had worked without issues." << std::endl << "But"
		<< " in cause of there was not any synchronizing mechanism they could start racing for 'cout' resource and text above could be messy."
		<< std::endl << "To prevent that situation we should use on of synchronization mechanism." << std::endl << std::endl;

	std::cout << "Mutex is primitive mechanism that put resource in locked position if it is in use right now." << std::endl
		<< "But it can`t be used if we have threads that must work in strict order." << std::endl << "Now i will create threads sequently" <<
		" but they will be use mutex for 'cout' resource." << std::endl << std::endl;

	std::mutex mainMutex; //Creating mutex

	std::vector<HANDLE> threads;

	for(int i = 0; i < 10; i++) {
		ThreadParams* params = new ThreadParams(mainMutex, i + 1); //Sending params to the structure for _beginthreadex
		HANDLE thread = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, &mutexWork, params, 0, NULL));
		threads.push_back(thread);
	}

	WaitForMultipleObjects(threads.size(), threads.data(), true, INFINITE);

	std::cout << "As you can see threads above used 'cout' resource without race and text will never be messed up." << std::endl
		<< "But they were not working in strict sequence and that is the disadvantage of mutex." << std::endl << std::endl;

	return 0;
}
