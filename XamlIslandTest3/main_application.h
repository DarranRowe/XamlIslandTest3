#pragma once

#ifndef _WINDOWS_
#define _WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#ifndef _VECTOR_
#include <vector>
#endif
#ifndef WINRT_Windows_Foundation_H
#include <winrt/Windows.Foundation.h>
#endif
#ifndef WINRT_Microsoft_UI_Xaml_H
#include <winrt/Microsoft.UI.Xaml.h>
#endif
#ifndef WINRT_Microsoft_UI_Xaml_Markup_H
#include <winrt/Microsoft.UI.Xaml.Markup.h>
#endif
#ifndef WINRT_Microsoft_UI_Xaml_Hosting_H
#include <winrt/Microsoft.UI.Xaml.Hosting.h>
#endif
#ifndef WINRT_XamlIslandTest3_H
#include "IslandApplication.h"
#endif

#include "application_base.h"
#include "window_base.h"

//This class is responsible for handling application related things.
//It handles the lifetime of the IslandApplication component. This includes the xaml host and providing 
//metadata providers and resources to the component.
//It handles the message pump and a simple method for message filtering required to get things to work.
class main_application : public application_base
{
public:
	//Gets the application instance, creates a new instance if one doesn't already exist.
	static main_application &get_application();

	~main_application();

	//Initialises the xaml host and gives the host the metadata providers it needs to query.
	void initialise_xaml_host(std::vector<winrt::Microsoft::UI::Xaml::Markup::IXamlMetadataProvider> const &);
	//Merges resources with the application's merged resource directory.
	void merge_resources(std::vector<winrt::Microsoft::UI::Xaml::ResourceDictionary> const &);

	//Removes all remaining messages in the message queue.
	//This is useful since crashes can occur if there are remaining messages when
	//the xaml host closes.
	void drain_message_queue();
	//Executes the main message loop/message pump for the application.
	int run_message_loop();
private:
	main_application();
	main_application(const main_application &) = delete;
	main_application(main_application &&) = delete;
	main_application &operator=(const main_application &) = delete;
	main_application &operator=(main_application &&) = delete;

	//Enumerates top level windows created on the same thread as this application class.
	//It then makes sure that it is derived from window_base, and if it is, requests
	//the pointer to the class.
	void detect_top_level_windows();
	//Obtains the collection of xaml sources from the window.
	void get_xaml_sources();
	//Does the message filtering for the xaml source.
	bool filter_message(const MSG &);

	winrt::XamlIslandTest3::IslandApplication m_islandapp = nullptr;
	std::vector<window_base *> m_windows{};
	std::vector<winrt::Microsoft::UI::Xaml::Hosting::DesktopWindowXamlSource> m_sources{};
	uint32_t m_creator_thread_id{};
};