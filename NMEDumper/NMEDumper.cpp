// NMEDumper.cpp : Определяет экспортируемые функции для DLL.
//

#include "pch.h"
#include "framework.h"
#include "NMEDumper.h"

#include <vector>
#include <string>

#define declareDummyVec(__type__, __name__) NMEDUMPER_X std::vector<__type__> __vec_of_ ## __name__ {}

declareDummyVec(void*, test);
declareDummyVec(std::string, test2);

template<typename T>
static constexpr inline T* getAddr(ULONG_PTR offs, ULONG_PTR base = 0x140000000ULL) {
	return reinterpret_cast<T*>(reinterpret_cast<decltype(offs)>(GetModuleHandleW(nullptr)) + (offs - base));
}

static void waitForDebugger() {
	while (!IsDebuggerPresent()) Sleep(2);
}

static void stuck() {
	for (;;);
}

static void doDump() {
	waitForDebugger();

	/*
	auto abaa = getAddr<std::vector<std::string>>(0x1404970b0);
	auto abab = getAddr<std::vector<void*>*>(0x140497090);
	auto abac = getAddr<std::vector<std::string>*>(0x140497070);
	*/

	char** a= getAddr<char*>(0x140496230);

	stuck();
}

NMEDUMPER_X int luaopen_NMEDumper(void* pLuaState) {
	UNREFERENCED_PARAMETER(pLuaState);
	doDump();
	return 0;
}

