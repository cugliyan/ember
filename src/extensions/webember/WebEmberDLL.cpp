/*
 Copyright (C) 2011 Peter Szucs

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software Foundation,
 Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/**
 * @file This file contains the DllMain and the interface for the WebEmber DLL, which will be exported.
 * @author Peter Szucs
 */

#include "WebEmberManager.h"

#include "framework/LoggingInstance.h"

#ifdef _WIN32
#include "platform/platform_windows.h"
#define WEBEMBER_EXPORT __declspec(dllexport) __cdecl
#elif __GNUC__ >= 4
//On GCC4, you can set visibility to minimalize exported abi ABI. The library will be faster and smaller.
//For more info, read: http://gcc.gnu.org/wiki/Visibility
//NOTE: Its not enabled by defaulat. You need to pass "-fvisibility=hidden" to the compiler.
#define WEBEMBER_EXPORT __attribute__((visibility("default")))
#else
#define WEBEMBER_EXPORT 
#endif

#include <sstream>

//wrap C++ functions with C interface: http://www.parashift.com/c++-faq-lite/mixing-c-and-cpp.html#faq-32.6
extern "C" int WEBEMBER_EXPORT StartWebEmber(const char* windowHandle, const char* prefix);
extern "C" void WEBEMBER_EXPORT QuitWebEmber();

using namespace Ember;

/**
 * @brief Start WebEmber as a child window of a given HWND.
 *
 * This function will run ember and will only return, when ember quits.
 *
 * @param hwnd The window handle of the owner.
 * @returns Returns the exit value of ember.
 */

int WEBEMBER_EXPORT StartWebEmber(const char* windowHandle, const char* prefix)
{
	return WebEmberManager::getSingleton().start(windowHandle, prefix);
}

/**
 * @brief Signals quit for WebEmber.
 *
 * This function will return immediately and ember will shut down before drawing the next frame.
 */
void WEBEMBER_EXPORT QuitWebEmber()
{
	WebEmberManager::getSingleton().quit();
}


#ifdef _WIN32
extern "C" BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD ul_reason_for_call, LPVOID lpReserved);

/**
 * @brief DllMain is the entry point for DLLs.
 *
 * Here you can do some initialization and uninitialization.
 * For more info, read: http://msdn.microsoft.com/en-us/library/ms682583%28v=vs.85%29.aspx
 */
BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch(ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		// DLL is loaded into the memory
		// if you return here FALSE the DLL will not be loaded and linking fails.
		new Ember::WebEmberManager();
		break;
	case DLL_PROCESS_DETACH:
		// DLL is unloaded from the memory
		delete Ember::WebEmberManager::getSingletonPtr();
		break;
	}
	return TRUE;
}
#else

extern "C" void initWebEmber(void);
extern "C" void deinitWebEmber(void);

//Set library constructor and destructor functions.
void __attribute__ ((constructor)) initWebEmber(void);
void __attribute__ ((destructor)) deinitWebEmber(void);

// Called when the library is loaded and before dlopen() returns
void initWebEmber(void)
{
	new Ember::WebEmberManager;
}

// Called when the library is unloaded and before dlclose()
void deinitWebEmber(void)
{
	delete Ember::WebEmberManager::getSingletonPtr();
}

#endif
