#pragma once

#include <sdkddkver.h>
#define _WIN32_LEAN_AND_MEAN
#define OEMRESOURCE
#include <Windows.h>

#undef GetCurrentTime

#include <string>
#include <filesystem>
#include <memory>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.System.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Hosting.h>
#include <winrt/Microsoft.UI.Xaml.Markup.h>
#include <winrt/Microsoft.UI.Xaml.XamlTypeInfo.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Controls.Primitives.h>

#include <wil/cppwinrt.h>
#include <wil/result.h>