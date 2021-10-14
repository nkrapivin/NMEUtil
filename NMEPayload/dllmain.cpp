// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "NMEPayload.h"

EXTERN_C BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReasonForCall, LPVOID lpReserved) {
	switch (dwReasonForCall) {
		case DLL_PROCESS_ATTACH: {
			NME_Start(hModule);
			break;
		}

		case DLL_PROCESS_DETACH: {
			NME_Quit();
			break;
		}

		default: {
			// don't care about threads...
			break;
		}
	}

	return TRUE;
}

