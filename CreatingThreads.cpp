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

struct ThreadSemaphoreParams {
	ThreadSemaphoreParams(HANDLE* sem, int number) : semaphores(sem), num(number) {};
	HANDLE* semaphores;
	int num;
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

unsigned __stdcall semaphoreWork(void* data) {
	ThreadSemaphoreParams* params = static_cast<ThreadSemaphoreParams*>(data);
	int number = params->num;
	HANDLE currentSemaphore = params->semaphores[number - 1];
	HANDLE nextSemaphore = params->semaphores[number];
	DWORD waitResult = WaitForSingleObject(currentSemaphore, 1);
	while (waitResult != WAIT_OBJECT_0) {
		waitResult = WaitForSingleObject(currentSemaphore, 1);
	}
	std::cout << std::endl << "My number is " << number << std::endl;
	ReleaseSemaphore(
		nextSemaphore, //Semaphore handle
		1, //Amount by which increase
		NULL //Variable, if need to save previous count (NULL if not)
	);
	return 0;
}

unsigned __stdcall eventWork(void* data) {
	HANDLE* eventHandle = static_cast<HANDLE*>(data);
	std::cout << std::endl << "Thread " << GetCurrentThreadId() << " is waiting for signal" << std::endl;
	WaitForSingleObject(eventHandle, INFINITE);
	std::cout << "Thread " << GetCurrentThreadId() << " is starting it`s work" << std::endl;
	Sleep(2000);
	std::cout << "Work is done" << std::endl;
	ResetEvent(eventHandle);
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

	for (auto thread : threads) {
		CloseHandle(thread);
	}

	std::cout << "Semaphore" << std::endl << std::endl
		<< "Semaphores can be used to run threads in strict order. Now i will do the same as previous but will use semaphores." << std::endl;

	HANDLE semaphores[10];
	for (int i = 0; i < 10; i++) {
		semaphores[i] = CreateSemaphore( //Creating semaphore
			NULL, //Inheritance attribute
			0, //Initial count
			1, //Max count
			NULL //Semaphore name
		);
	}

	threads.clear();

	for (int i = 0; i < 10; i++) {
		ThreadSemaphoreParams* params = new ThreadSemaphoreParams(semaphores, i + 1);
		HANDLE thread = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, &semaphoreWork, params, 0, NULL));
		threads.push_back(thread);
	}

	ReleaseSemaphore(semaphores[0], 1, NULL);

	WaitForMultipleObjects(threads.size(), threads.data(), true, INFINITE);

	for (auto thread : threads) {
		CloseHandle(thread);
	}
	for (int i = 0; i < 10; i++) {
		CloseHandle(semaphores[i]);
	}

	std::cout << std::endl << "As you can see threads had worked in order, that`s because semaphores can be changed in other threads."
		<< std::endl << "Unlike semaphores mutexes can be released only in thread where they were locked." << std::endl << std::endl;

	std::cout << "Events" << std::endl;
	std::cout << std::endl << "Events can be used for synchronization too." << std::endl
		<< "They used when we need to send a signal for thread, that it can use resources." << std::endl << std::endl;

	HANDLE event = CreateEvent(
		NULL, //Inheritance attribute
		true, //Is manual reset
		false, //Initial state
		NULL //Name of event
	);

	HANDLE eventThread = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, &eventWork, event, 0, NULL));

	std::cout << "Main thread is using the resource right now..." << std::endl;

	Sleep(5000);

	std::cout << std::endl << "Main thread will now free the resource." << std::endl;
	SetEvent(event);

	WaitForSingleObject(eventThread, INFINITE);

	CloseHandle(eventThread);
	CloseHandle(event);

	return 0;
}
