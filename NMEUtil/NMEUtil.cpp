// NMEUtil.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

int errorOut(const wchar_t* str, /* DWORD dwLastErrorCode */ ...) {
	va_list args{};
	int rr{ -1024 };
	wchar_t buff[512]{ '\0' };

	va_start(args, str);
	rr = vswprintf_s(buff, str, args);
	va_end(args);

	rr = MessageBoxW(GetActiveWindow(), buff, L"NMEUtil: An Error!", MB_ICONERROR);
	return 1; // 1 - failure.
}

int main(int argc, char* argv[], char* envp[]) {
	// edit these if you wish:
	const WCHAR dllName[]{ L"NMEPayload.dll" };
	const WCHAR processName[]{ L"PlayPrime2D.exe" };
	const SIZE_T dllSize{ sizeof(dllName) };

	// important variables:
	STARTUPINFOW siw{ sizeof(STARTUPINFOW) };
	PROCESS_INFORMATION pi{ nullptr };
	DWORD tid{ 0 }; // thread id
	SIZE_T wrote{ 0 }; // how many bytes wrote
	HANDLE dllth{ nullptr }; // dll loader thread
	BOOL ok{ FALSE }; // ok flag
	LPVOID addr{ nullptr }; // dll path address
	DWORD exitcode{ 0 }; // 0 - success, 1 - failure.

	// since I use a Russian Windows installation
	// I have to execute these lines first:
	ok = SetConsoleOutputCP(CP_UTF8);
	if (!ok) return errorOut(L"SetConsoleOutputCP failed. Err=0x%lX", GetLastError());
	ok = SetConsoleCP(CP_UTF8);
	if (!ok) return errorOut(L"SetConsoleCP failed. Err=0x%lX", GetLastError());

	// spawn the process:
	ok = CreateProcessW(
		processName,
		GetCommandLineW(), // pass-through all console args.
		nullptr,
		nullptr,
		FALSE,
		CREATE_UNICODE_ENVIRONMENT | CREATE_SUSPENDED | DETACHED_PROCESS,
		nullptr,
		nullptr,
		&siw,
		&pi
	);

	if (!ok) return errorOut(L"CreateProcessW failed. Err=0x%lX", GetLastError());

	// inject the dll:
	addr = VirtualAllocEx(pi.hProcess, nullptr, dllSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!addr) return errorOut(L"VirtualAllocEx failed. Err=0x%lX", GetLastError());
	ok = WriteProcessMemory(pi.hProcess, addr, dllName, dllSize, &wrote) && wrote > 0;
	if (!ok) return errorOut(L"WriteProcessMemory failed. Err=0x%lX", GetLastError());
	dllth = CreateRemoteThread(pi.hProcess, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(&LoadLibraryW), addr, 0, &tid);
	if (!dllth) return errorOut(L"CreateRemoteThread failed. Err=0x%lX", GetLastError());
	ok = WaitForSingleObject(dllth, INFINITE) == WAIT_OBJECT_0;
	if (!ok) return errorOut(L"WaitForSingleObject failed. Err=0x%lX", GetLastError());
	ok = CloseHandle(dllth);
	if (!ok) return errorOut(L"CloseHandle failed. Err=0x%lX", GetLastError());
	ok = VirtualFreeEx(pi.hProcess, addr, 0, MEM_RELEASE);
	if (!ok) return errorOut(L"VirtualFreeEx failed. Err=0x%lX", GetLastError());

	// actually resume the thing:
	ok = ResumeThread(pi.hThread) != ((DWORD)-1);
	if (!ok) return errorOut(L"ResumeThread failed. Err=0x%lX", GetLastError());

	// close everything!
	if (siw.hStdOutput) ok = ok && CloseHandle(siw.hStdOutput);
	if (siw.hStdInput)  ok = ok && CloseHandle(siw.hStdInput);
	if (siw.hStdError)  ok = ok && CloseHandle(siw.hStdError);
	if (pi.hThread)     ok = ok && CloseHandle(pi.hThread);
	if (pi.hProcess)    ok = ok && CloseHandle(pi.hProcess);

	if (!ok) return errorOut(L"CloseHandle failed. Err=%lX", GetLastError());

	return exitcode;
}

