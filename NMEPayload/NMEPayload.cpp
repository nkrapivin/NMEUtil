// NMEPayload.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "NMEPayload.h"

#include <type_traits>
#include <string>
#include <fstream>
#include <iostream>
#include "lua-5.4.1\src\lua.hpp"
#include "minhook\include\MinHook.h"

void __MH_ASSERT_FAIL_IMPL(int line, const char* file, const char* func) {
	static char msgbuf[4096]{ L'\0' };

	memset(msgbuf, 0, sizeof(msgbuf));
	snprintf(msgbuf, sizeof(msgbuf) - 1, "An error:\r\nFile: %s\r\nFunction: %s\r\nLine: %d", file, func, line);
	MessageBoxA(GetActiveWindow(), msgbuf, "NMEPayload: Fatal Error", MB_ICONERROR);
	Sleep(1000);
	abort();
}


#define MH_ASSERT(__MH_STATUS__) if (((__MH_STATUS__) != (MH_OK))) __MH_ASSERT_FAIL_IMPL(__LINE__, __FILE__, __FUNCTION__)

// VT100 codes:
#define NIKMSG_OK   "[\x1b[32m+\x1b[0m]"
#define NIKMSG_ERR  "[\x1b[31m!\x1b[0m]"
#define NIKMSG_INFO "[\x1b[34m.\x1b[0m]"

typedef int(*printf_t)(const char* _fmt, ...);
typedef FILE*(__cdecl* __aif_t)(unsigned _Ix);
static __aif_t* g_acrt_iob_func_game{ nullptr };
#define GAME_stdin  ((*g_acrt_iob_func_game)(0))
#define GAME_stdout ((*g_acrt_iob_func_game)(1))
#define GAME_stderr ((*g_acrt_iob_func_game)(2))

static printf_t prf{ nullptr };

typedef void(*NMInitLuaForProject)(void** projectData);
typedef void(*NMLuaDoCode)(lua_State* L, char* codeString, char* funcName);
typedef int(*NikLuaLoadT)(lua_State* L, lua_Reader reader, void* data, const char* chunkname, const char* mode);

static NMInitLuaForProject trampolineP{ nullptr };
static NMLuaDoCode trampolineC{ nullptr };
static NikLuaLoadT trampolineL{ nullptr };
static lua_Reader LuaReaderOriginal{ nullptr };
static FILE* LuaDumperHandle{ nullptr };
static lua_State** g_pL{ nullptr };
static bool g_DoHookFiles{ false };

static HMODULE g_hDLL{ nullptr };

bool EnableVT100(HANDLE hCon, bool isStdin) {
	DWORD dwCMode{ 0 };

	if (GetConsoleMode(hCon, &dwCMode)) {
		// yes these are different flags, and yes you can't use the _INPUT flag on an stdout handle...
		dwCMode |= isStdin ? ENABLE_VIRTUAL_TERMINAL_INPUT : ENABLE_VIRTUAL_TERMINAL_PROCESSING;

		if (SetConsoleMode(hCon, dwCMode)) {
			return true;
		}
		else {
			prf(u8"Failed to set VT100 console mode, err=0x%lX, ignore this message.\r\n", GetLastError());
		}
	}
	else {
		prf(u8"Failed to get console mode, err=0x%lX, ignore this message.\r\n", GetLastError());
	}

	return false;
}

// https://stackoverflow.com/questions/15543571/allocconsole-not-displaying-cout
void CreateConsole() {
	if (!AllocConsole()) {
		// Add some error handling here.
		// You can call GetLastError() to get more info about the error.
		std::abort();
	}

	typedef FILE* (__cdecl* _ms_wfreopen_t)(wchar_t const* filename, wchar_t const* mode, FILE* fs);
	
	HMODULE ms = LoadLibraryW(L"MSVCRT.DLL");
	if (!ms) std::abort();
	_ms_wfreopen_t wfreop = reinterpret_cast<_ms_wfreopen_t>(GetProcAddress(ms, "_wfreopen"));
	prf = reinterpret_cast<printf_t>(GetProcAddress(ms, "printf"));
	if (!wfreop || !prf) std::abort();
	
	// std::cout, std::clog, std::cerr, std::cin
	wfreop(L"CONOUT$", L"w", GAME_stdout);
	wfreop(L"CONOUT$", L"w", GAME_stderr);
	wfreop(L"CONIN$", L"r", GAME_stdin);

	// std::wcout, std::wclog, std::wcerr, std::wcin
	HANDLE hConOut = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	HANDLE hConIn = CreateFileW(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
	SetStdHandle(STD_ERROR_HANDLE, hConOut);
	SetStdHandle(STD_INPUT_HANDLE, hConIn);

	// russian language fix:
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);

	// enable VT100 support:
	//EnableVT100(hConIn, true);
	EnableVT100(hConOut, false);

	// try outputting something:
	SetConsoleTitleW(L"NMEPayload Debug Console Window");
	prf(u8"[-Testing the console--------------------------------------------------]\r\n");
	prf(u8"NMEPayload by nkrapivindev.\r\n");
	prf(u8"И помните - шифрование данных игры это ВСЕГДА плохо, КРОМЕ мультиплеера.\r\n");
	prf(u8"[-Game stuff below-----------------------------------------------------]\r\n");
}

void NMEConsole_Start() {
	CreateConsole();
	//...
}

// a simple wrapper to resolve addresses by ghidra offsets.
template<typename T>
constexpr T* NME_Addr(ULONG_PTR offs) {
	return reinterpret_cast<T*>(reinterpret_cast<decltype(offs)>(GetModuleHandleW(nullptr)) + offs);
}

const char* LuaErrToString(int errcode) {
	switch (errcode) {
		case LUA_OK: return "LUA_OK";
		case LUA_YIELD: return "LUA_YIELD";
		case LUA_ERRRUN: return "LUA_ERRRUN";
		case LUA_ERRSYNTAX: return "LUA_ERRSYNTAX";
		case LUA_ERRMEM: return "LUA_ERRMEM";
		case LUA_ERRERR: return "LUA_ERRERR";
		case LUA_ERRFILE: return "LUA_ERRFILE";
		default: return "<UNKNOWN>";
	}
}

void NikInitLuaForProject(void** projectData) {
	int lret{ -1024 }; // just so I can see the changes in a debugger.
	//NMEConsole_Start();
	trampolineP(projectData);
	
	const char* emsg{ "<no error?>" };
	size_t outlen{ 0 };
	auto luatostring = ((const char* (*)(lua_State * L, int idx, size_t * len))(NME_Addr<VOID>(0xb5026)));

	g_DoHookFiles = true;

	// luaL_loadfile()
	lret = (((int(*)(lua_State *L, const char *filename, const char *mode))(NME_Addr<VOID>(0xb95ec)))(*g_pL, "Initial.lua", nullptr));
	if (lret != LUA_OK) {
		// lua_tolstring()
		emsg = (luatostring(*g_pL, -1, &outlen));
		prf(NIKMSG_ERR
			" There was an error when loading the initial lua file=%s:\r\n", LuaErrToString(lret));
		prf("    %s\r\n", emsg);

		Sleep(2 * 1000);
		return;
	}
	// lua_pcallk()
	lret = (((int(*)(lua_State *L, int nargs, int nresults, int errfunc, lua_KContext ctx, lua_KFunction k))(NME_Addr<VOID>(0xb6c5f)))(*g_pL, 0, 0, 0, 0, nullptr));
	if (lret != LUA_OK) {
		// lua_tolstring()
		emsg = (luatostring(*g_pL, -1, &outlen));
		prf(NIKMSG_ERR
			" There was an error when executing the initial lua file=%s:\r\n", LuaErrToString(lret));
		prf("    %s\r\n", emsg);
		
		Sleep(10 * 1000);
		abort();
		return;
	}
	
	prf(NIKMSG_OK
		" Initial lua file OK. :)\r\n");
}

void NikLuaDoCode(lua_State* L, char* codeString, char* funcName) {
	char* actualcode{ codeString };
	bool overwrote{ false };
	size_t slen{ strlen(codeString) };
	
	// troid why, do you think I'm an idiot or something?
	const char* toWIPEOUT = "os=nil;io=nil;package=nil;loadfile=nil;load=nil;require=nil;";
	char* towipe{ strstr(codeString, toWIPEOUT) };
	
	if (towipe) {
		// can't modify codeString because it's constant
		// doing so will result in a WinAPI exception, so we make a copy of it.
		auto towipeoffs{ towipe - codeString };
		actualcode = new char[slen + 1]{ '\0' };
		overwrote = true;
		memcpy(actualcode, codeString, slen);
		// replace the string with spaces so the funny stuff won't get niled.
		memset(actualcode + towipeoffs, ' ', strlen(toWIPEOUT));
	}

	trampolineC(L, actualcode, funcName);

	// free the string copy if overwrote.
	if (overwrote) {
		delete[] actualcode;
		actualcode = nullptr;
	}
}

const char* NikLuaReaderHook(lua_State* L, void* ud, size_t* sz) {
	// skip the original file by reading it till end.
	while (LuaReaderOriginal(L, ud, sz));

	// change the size if you wish...
	static char readbuffer[512]{ '\0' };
	
	// read the whole buffer at once.
	size_t readread{ fread_s(readbuffer, sizeof(readbuffer), sizeof(readbuffer[0]), sizeof(readbuffer), LuaDumperHandle) };
	*sz = readread;

	// done?
	if (readread == 0 && feof(LuaDumperHandle)) {
		fclose(LuaDumperHandle);
		LuaDumperHandle = nullptr;
	}

	return readbuffer;
}

int NikLuaLoad(lua_State* L, lua_Reader reader, void* data, const char* chunkname, const char* mode) {
	if (!g_DoHookFiles) return trampolineL(L, reader, data, chunkname, mode);

	int ret{ -1024 };

	// prepare:
	LuaReaderOriginal = reader;
	lua_Reader touse{ reader };

	FILE* f{ nullptr };
	errno_t ok{ fopen_s(&f, chunkname, "rb") };
	if (f && ok == 0) {
		LuaDumperHandle = f;
		touse = &NikLuaReaderHook;
		prf(NIKMSG_OK
			" Hooking file '%s'...\r\n", chunkname);
		// TODO: do something next?
	}
	else {
		// TODO: the file does not exist
		// so technically we shouldn't do anything?
		// let's print the filename for now:
		prf(NIKMSG_INFO
			" The game tried to load lua file '%s' but it doesn't exist on disk.\r\n", chunkname);
	}
	

	ret = trampolineL(L, touse, data, chunkname, mode);
	return ret;
}

void NMEPayload_Start() {
	// address to lua_State* (must be de-refed to get an actual lua_State*)
	g_pL = NME_Addr<lua_State*>(0x4965e0);
	g_acrt_iob_func_game = NME_Addr<__aif_t>(0x396920);
	//g_Project = NME_Addr<void*>(0x496610);
	//L = *pL;

	//while (!IsDebuggerPresent());

	MH_ASSERT(MH_Initialize());

	MH_ASSERT(
		MH_CreateHook(
			NME_Addr<VOID>(0x645a0),
			reinterpret_cast<LPVOID >(&NikLuaDoCode),
			reinterpret_cast<LPVOID*>(&trampolineC)
		)
	);

	MH_ASSERT(
		MH_CreateHook(
			NME_Addr<VOID>(0x172f0),
			reinterpret_cast<LPVOID >(&NikInitLuaForProject),
			reinterpret_cast<LPVOID*>(&trampolineP)
		)
	);

	MH_ASSERT(
		MH_CreateHook(
			NME_Addr<VOID>(0xb6e47),
			reinterpret_cast<LPVOID >(&NikLuaLoad),
			reinterpret_cast<LPVOID*>(&trampolineL)
		)
	);

	MH_ASSERT(MH_EnableHook(MH_ALL_HOOKS));
}

void NMEConsole_Quit() {
	prf(NIKMSG_INFO
		" Bye, I'll miss you :<\r\n");
	Sleep(1000);
	FreeConsole();
	// the game should end here.
}

void NMEPayload_Quit() {
	// free everything carefully.
	prf(NIKMSG_OK
		" Removing hooks...\r\n");
	// ... in reverse order
	MH_ASSERT(MH_RemoveHook(NME_Addr<VOID>(0xb6e47)));
	MH_ASSERT(MH_RemoveHook(NME_Addr<VOID>(0x172f0)));
	MH_ASSERT(MH_RemoveHook(NME_Addr<VOID>(0x645a0)));
	// bye MinHook :<
	MH_ASSERT(MH_Uninitialize());
}

void NME_Start(HMODULE hMe) {
	g_hDLL = hMe;
	NMEPayload_Start();
	NMEConsole_Start();
}

void NME_Quit() {
	NMEPayload_Quit();
	NMEConsole_Quit();
}

