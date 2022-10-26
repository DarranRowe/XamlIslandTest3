#include "pch.h"

#include "wappsdkbootstrap.h"
#include "main_application.h"
#include "main_window.h"

//Controls Com/WinRT lifetime.
struct com_init
{
	com_init()
	{
		winrt::init_apartment(winrt::apartment_type::single_threaded);
	}

	~com_init()
	{
		winrt::uninit_apartment();
	}
};
com_init g_com_init;

namespace muxx = winrt::Microsoft::UI::Xaml::XamlTypeInfo;
namespace muxc = winrt::Microsoft::UI::Xaml::Controls;

void except_filter()
{
	try
	{
		throw;
	}
	catch (winrt::hresult_error &)
	{
	}
	catch (wil::ResultException &)
	{
	}
	catch (std::exception &)
	{
	}
}

int application_main(HINSTANCE inst, LPWSTR, int cmdshow)
{
	int main_return = 0;
	//This scope makes sure that the main_app reference is out of scope
	//when we destroy it. We don't want any dangling references.
	{
		//Load the WinUI metadata provider.
		muxx::XamlControlsXamlMetaDataProvider metadata_provider;
		//Obtains the application.
		main_application &app = main_application::get_application();
		//Initialise the application.
		//This passes the metadata provider through to the Xaml application.
		app.initialise_xaml_host({ metadata_provider });
		//Loads the WinUI control resources.
		muxc::XamlControlsResources resources;
		//Merge the resources into the xaml merged dictionaries.
		app.merge_resources({ resources });

		//Create and show the main window.
		main_window window(inst);
		window.create_window(cmdshow);
		main_return = app.run_message_loop();
	}

	main_application::get_application().close();
	return main_return;
}

int WINAPI wWinMain(_In_ HINSTANCE inst, _In_opt_ HINSTANCE, _In_ LPWSTR cmdline, _In_ int cmdshow)
{
	int result = 0;

	try
	{
		if (!init_wappsdk())
		{
			return -255;
		}

		result = application_main(inst, cmdline, cmdshow);

		cleanup_wappsdk();
	}
	catch (...)
	{
		except_filter();
	}
	return result;
}