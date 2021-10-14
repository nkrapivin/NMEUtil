// NMEUtil.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

int main() {
	STARTUPINFOW siw{ sizeof(STARTUPINFOW) };
	PROCESS_INFORMATION pi{ nullptr };
	DWORD tid{ 0 };
	SIZE_T wrote{ 0 };
	HANDLE dllth{ nullptr };
	const WCHAR dllName[] = L"NMEPayload.dll";
	const SIZE_T dllSize{ sizeof(dllName) };
	BOOL ok{ FALSE };
	LPVOID addr{ nullptr };
	DWORD exitcode{ 0 };
	DWORD procflags{ CREATE_UNICODE_ENVIRONMENT | CREATE_SUSPENDED | DETACHED_PROCESS }; // must use unicode.

	// since I use a Russian Windows installation
	// I have to execute these lines first:
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);

	ok = CreateProcessW(
		L"PlayPrime2D.exe",
		GetCommandLineW(), // pass-through all console args.
		nullptr,
		nullptr,
		FALSE,
		procflags,
		nullptr,
		nullptr,
		&siw,
		&pi
	);

	if (ok) {
		// inject the dll:
		addr = VirtualAllocEx(pi.hProcess, nullptr, dllSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		ok = WriteProcessMemory(pi.hProcess, addr, dllName, dllSize, &wrote);
		dllth = CreateRemoteThread(pi.hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)&LoadLibraryW, addr, 0, &tid);
		WaitForSingleObject(dllth, INFINITE);
		CloseHandle(dllth);

		// actually resume the thing:
		ResumeThread(pi.hThread);

		// close everything!
		CloseHandle(siw.hStdOutput);
		CloseHandle(siw.hStdInput);
		CloseHandle(siw.hStdError);
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);

		return exitcode;
	}

	MessageBoxW(nullptr, L"Failed to create the game process!", L"NMEUtil Fatal Error", MB_OK | MB_ICONERROR);
	return exitcode;
}

