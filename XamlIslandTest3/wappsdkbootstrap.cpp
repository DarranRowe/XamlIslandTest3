#include "pch.h"

#include <strsafe.h>
#include <WindowsAppSDK-VersionInfo.h>
#include <wil/result.h>
#include <MddBootstrap.h>

static bool g_wappsdk_initialised = false;

bool init_wappsdk()
{
	auto result = MddBootstrapInitialize2(WINDOWSAPPSDK_RELEASE_MAJORMINOR, WINDOWSAPPSDK_RELEASE_VERSION_TAG_W, PACKAGE_VERSION{}, MddBootstrapInitializeOptions_OnNoMatch_ShowUI | MddBootstrapInitializeOptions_OnError_DebugBreak_IfDebuggerAttached);
	if (FAILED(result))
	{
		return false;
	}

	g_wappsdk_initialised = true;
	return true;
}
void cleanup_wappsdk()
{
	if (g_wappsdk_initialised == true)
	{
		MddBootstrapShutdown();
	}
}