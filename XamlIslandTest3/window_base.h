#pragma once

#ifndef _WINDOWS_
#define _WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#ifndef WINRT_Microsoft_UI_Xaml_H
#include <winrt/Microsoft.UI.Xaml.h>
#endif
#ifndef WINRT_Microsoft_UI_Xaml_Hosting_H
#include <winrt/Microsoft.UI.Xaml.Hosting.h>
#endif

//Message used to query if this is a window that derives from window_base;
#ifndef WM_USER_QUERY_WINDOWBASE
#define WM_USER_QUERY_WINDOWBASE WM_USER + 10
#endif

//Message used to obtain the window_base * from the window handle.
//The window class doesn't change ownership, what is returned is
//a non owning pointer.
#ifndef WM_USER_GET_WINDOWBASE_POINTER
#define WM_USER_GET_WINDOWBASE_POINTER WM_USER + 11
#endif

//Message used to verify that the pointer we use is tied to the window.
#ifndef WM_USER_VERIFY_POINTER
#define WM_USER_VERIFY_POINTER WM_USER + 12
#endif

//Value returned to indicate that this is a window that derives from window_base.
constexpr uint32_t query_window_base_identified = 0xFEEDF00D;
//Value returned to indicate that the pointer we verified is the same as the
//pointer to the class that backs the window we are verifying against.
constexpr uint32_t verify_window_base_pointer_match = 0x0000900D;
//Value returned to indicate that the pointer we verified is not the same as the
//pointer to the class that backs the window we are verifying against.
constexpr uint32_t verify_window_base_pointer_no_match = 0x0000BAAD;

//Base class that our windows derive from.
class window_base
{
public:
	//Obtains all of the xaml sources from the window.
	//This is used for the message filtering.
	std::vector<winrt::Microsoft::UI::Xaml::Hosting::DesktopWindowXamlSource> get_xaml_sources();
	//Obtains the handle for the window that this class represents.
	HWND get_handle() const;
	//Used to move the focus around the controls contained by the window that this class represents.
	bool focus_navigate(MSG *);
protected:
	//Sets the handle for this class.
	//This must only be called once when the window initialises.
	void set_handle(HWND);

	//Creates a DesktopWindowXamlSource object from the given xaml element.
	HWND create_desktop_window_xaml_source(DWORD extra_styles, const winrt::Microsoft::UI::Xaml::UIElement &);
	//Destroys all DesktopWindowXamlSource objects cached by this class.
	void clear_xaml_islands();
private:
	HWND m_handle = nullptr;

	//Helper function to get a window handle from a DesktopWindowXamlSource object.
	HWND get_handle(winrt::Microsoft::UI::Xaml::Hosting::DesktopWindowXamlSource const &);
	//Helper function to get a window handle from a DesktopWindowXamlSource object.
	//It attaches the source to a window while it is doing this.
	HWND get_handle_and_attach(winrt::Microsoft::UI::Xaml::Hosting::DesktopWindowXamlSource const &, HWND);
	//Checks cached xaml sources to see if one of them currently has focus.
	winrt::Microsoft::UI::Xaml::Hosting::DesktopWindowXamlSource get_focused_island();
	//Take focus requested event handler. This event fires when when focus changes from a xaml island to a different control.
	void on_take_focus_requested(winrt::Microsoft::UI::Xaml::Hosting::DesktopWindowXamlSource const &, winrt::Microsoft::UI::Xaml::Hosting::DesktopWindowXamlSourceTakeFocusRequestedEventArgs const &);
	//Got focus event handler. This event (allegedly) fires when the focus changes to a xaml island.
	void on_got_focus(winrt::Microsoft::UI::Xaml::Hosting::DesktopWindowXamlSource const &, winrt::Microsoft::UI::Xaml::Hosting::DesktopWindowXamlSourceGotFocusEventArgs const &);
	//Obtains which island is to get the focus next.
	winrt::Microsoft::UI::Xaml::Hosting::DesktopWindowXamlSource get_next_focused_island(const MSG*);
	//Moves the focus between controlls.
	bool navigate_focus(MSG *);

	winrt::guid m_last_focus_request_id{};
	std::map<HWND, winrt::event_token> m_take_focus_event_tokens;
	std::map<HWND, winrt::event_token> m_got_focus_event_tokens;
	std::vector<winrt::Microsoft::UI::Xaml::Hosting::DesktopWindowXamlSource> m_xaml_sources;
};

//Loads xaml content from a file on the filesystem.
winrt::Microsoft::UI::Xaml::UIElement LoadControlFromFile(std::wstring const &);
//Loads xaml content from a Windows API resource.
//Resource must be type 255.
winrt::Microsoft::UI::Xaml::UIElement LoadControlFromResource(uint16_t);