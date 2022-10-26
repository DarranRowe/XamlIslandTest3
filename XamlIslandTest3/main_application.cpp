#include "pch.h"
#include "main_application.h"

#include <microsoft.ui.xaml.hosting.desktopwindowxamlsource.h>

namespace wf = winrt::Windows::Foundation;
namespace wfc = winrt::Windows::Foundation::Collections;
namespace mux = winrt::Microsoft::UI::Xaml;
namespace muxm = winrt::Microsoft::UI::Xaml::Markup;
namespace muxh = winrt::Microsoft::UI::Xaml::Hosting;

//The thread id is required for when we look for the top level windows.
//Only the ones that are created on the same thread as this application are
//considered.
main_application::main_application() : m_creator_thread_id(GetCurrentThreadId())
{
}

main_application &main_application::get_application()
{
	if (application_base::contains_application())
	{
		return static_cast<main_application &>(get_stored_application());
	}

	std::unique_ptr<main_application> ptr(new main_application());

	application_base::set_application_instance(ptr.release());
	return static_cast<main_application &>(get_stored_application());
}

main_application::~main_application()
{
	//make sure the message queue/dispatcher queue is empty
	//if this is not done, there may be a crash on process exit.
	drain_message_queue();
	if (m_islandapp)
	{
		m_islandapp.Close();
		m_islandapp = nullptr;
	}
}

//Take a copy of the metadata providers and then forward them to the IslandApplication component.
void main_application::initialise_xaml_host(std::vector<muxm::IXamlMetadataProvider> const &metadata_providers)
{
	std::vector<muxm::IXamlMetadataProvider> providers;
	for (auto &prov : metadata_providers)
	{
		providers.push_back(prov);
	}
	wfc::IVector<muxm::IXamlMetadataProvider> provs = winrt::multi_threaded_vector<muxm::IXamlMetadataProvider>(std::move(providers));
	m_islandapp = winrt::XamlIslandTest3::IslandApplication(provs);
}

//Merge the resource dictionaries into the merged dictionaries for the IslandApplication component.
void main_application::merge_resources(std::vector<mux::ResourceDictionary> const &merge_dictionaries)
{
	if (m_islandapp == nullptr)
	{
		return;
	}

	for (auto &dictionary : merge_dictionaries)
	{
		m_islandapp.Resources().MergedDictionaries().Append(dictionary);
	}
}

//Clears any remaining message in the message queue.
void main_application::drain_message_queue()
{
	MSG msg{};

	while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		DispatchMessageW(&msg);
	}
}

//Finds any top level window on the same thread that this application class was
//created on.
void main_application::detect_top_level_windows()
{
	std::vector<HWND> windows{};
	//Enumerates top level windows.
	EnumThreadWindows(m_creator_thread_id, [](HWND hwnd, LPARAM param) -> BOOL
		{
			std::vector<HWND> &windows = *reinterpret_cast<std::vector<HWND> *>(param);

			//Place the window handle into a vector.
			windows.push_back(hwnd);

			return TRUE;
		}, reinterpret_cast<LPARAM>(std::addressof(windows)));

	for (auto &window : windows)
	{
		//This message queries whether the window handle is backed by
		//the window_base class implemented in this application.
		//We specifically look for a value which isn't a 0 or a 1 to try to avoid potential
		//collisions.
		auto result = SendMessageW(window, WM_USER_QUERY_WINDOWBASE, 0, 0);
		if (result == query_window_base_identified)
		{
			//If the window replies correctly, ask for the pointer to the window_base object.
			//The pointer is then cached.
			auto *base = reinterpret_cast<window_base *>(SendMessageW(window, WM_USER_GET_WINDOWBASE_POINTER, 0, 0));
			_ASSERTE(base != nullptr);
			m_windows.push_back(base);
		}
	}
}

void main_application::get_xaml_sources()
{
	//For every window_base pointer that we have, request any xaml sources from them.
	//This is then used to filter the messages.
	for (auto &window : m_windows)
	{
		const auto &sources = window->get_xaml_sources();

		m_sources.insert(m_sources.end(), sources.begin(), sources.end());
	}
}

//For every xaml source that we have obtained from the top level windows, obtain
//the IDesktopWindowXamlSourceNative2 interface and call PreTranslateMessage.
//This function is used to route Xaml messages to the xaml sources so they can
//function correctly.
bool main_application::filter_message(const MSG &msg)
{
	for (auto &source : m_sources)
	{
		BOOL handled = FALSE;
		auto native = source.as<IDesktopWindowXamlSourceNative>();

		winrt::check_hresult(native->PreTranslateMessage(&msg, &handled));
		if (handled != FALSE)
		{
			return true;
		}
	}
	return false;
}

//Runs the message loop/message pump.
int main_application::run_message_loop()
{
	MSG msg{};

	//Gets the windows and related xaml sources.
	//This has the obvious problem of being static, so adding any windows or
	//xaml sources after the message pump starts isn't possible with this code.
	//There are a couple of obvious solutions to this, including posting thread
	//messages or making the application a singleton.
	detect_top_level_windows();
	get_xaml_sources();

	while (GetMessageW(&msg, nullptr, 0, 0))
	{
		//Filter the xaml messages first.
		//If the message isn't handled by the xaml source, then we
		//carry on with the message processing.
		if (!filter_message(msg))
		{
			//Check for keyboard navigation next.
			//If navigation doesn't occur then carry on with message
			//processing.
			bool navigated = false;
			for (auto &window : m_windows)
			{
				navigated = window->focus_navigate(&msg);
				if (navigated)
				{
					break;
				}
			}
			if (!navigated)
			{
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
			}
		}
	}
	//The following has a known issue.
	//If the main window is closed while another top level window is open, there will
	//be a short period of time where there are windows visible but not properly routing
	//navigation messages or filtering the messages for the xaml sources.
	//This is fine for the sample, but a method of dealing with this needs to be devised.
	//Clear the cached xaml sources.
	m_sources.clear();
	//Clear the cached window_base pointers here.
	m_windows.clear();
	return static_cast<int>(msg.wParam);
}