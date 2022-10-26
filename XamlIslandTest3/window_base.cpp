#include "pch.h"
#include "window_base.h"

#include <microsoft.ui.xaml.hosting.desktopwindowxamlsource.h>

namespace wf = winrt::Windows::Foundation;
namespace mux = winrt::Microsoft::UI::Xaml;
namespace muxh = winrt::Microsoft::UI::Xaml::Hosting;
namespace muxm = winrt::Microsoft::UI::Xaml::Markup;

constexpr uint16_t xamlresourcetype = 255;
constexpr auto static invalid_reason = static_cast<muxh::XamlSourceFocusNavigationReason>(-1);
constexpr static WPARAM invalid_key = static_cast<WPARAM>(-1);

HWND window_base::get_handle() const
{
	return m_handle;
}
void window_base::set_handle(HWND handle)
{
	m_handle = handle;
}

//Takes a VK code from a WM_KEYDOWN message and converts it to
//a xaml XamlSourceFocusNavigationReason value.
//Tab goes to first or last depending on whether shift has been pressed.
muxh::XamlSourceFocusNavigationReason get_reason_from_key(WPARAM key)
{
	auto reason = invalid_reason;
	switch(key)
	{
	case VK_TAB:
	{
		byte keyboard_state[256] = {};
		THROW_IF_WIN32_BOOL_FALSE(GetKeyboardState(keyboard_state));
		reason = (keyboard_state[VK_SHIFT] & 0x80) ? muxh::XamlSourceFocusNavigationReason::Last : muxh::XamlSourceFocusNavigationReason::First;
		break;
	}
	case VK_LEFT:
	{
		reason = muxh::XamlSourceFocusNavigationReason::Left;
		break;
	}
	case VK_RIGHT:
	{
		reason = muxh::XamlSourceFocusNavigationReason::Right;
		break;
	}
	case VK_UP:
	{
		reason = muxh::XamlSourceFocusNavigationReason::Up;
		break;
	}
	case VK_DOWN:
	{
		reason = muxh::XamlSourceFocusNavigationReason::Down;
		break;
	}
	}

	return reason;
}
//Takes a XamlSourceFocusNavigationReason value and converts it to a VK
//code.
WPARAM get_key_from_reason(muxh::XamlSourceFocusNavigationReason reason)
{
	auto key = invalid_key;

	switch (reason)
	{
	case muxh::XamlSourceFocusNavigationReason::Last:
	{
		key = VK_TAB;
		break;
	}
	case muxh::XamlSourceFocusNavigationReason::First:
	{
		key = VK_TAB;
		break;
	}
	case muxh::XamlSourceFocusNavigationReason::Left:
	{
		key = VK_LEFT;
		break;
	}
	case muxh::XamlSourceFocusNavigationReason::Right:
	{
		key = VK_RIGHT;
		break;
	}
	case muxh::XamlSourceFocusNavigationReason::Up:
	{
		key = VK_UP;
		break;
	}
	case muxh::XamlSourceFocusNavigationReason::Down:
	{
		key = VK_DOWN;
		break;
	}
	}

	return key;
}

muxh::DesktopWindowXamlSource window_base::get_next_focused_island(const MSG *msg)
{
	//This only happens if we are working with a key down mesage.
	if (msg->message == WM_KEYDOWN)
	{
		//Obtains the reason from the WM_KEYDOWN's virtual key code.
		const auto key = msg->wParam;
		auto reason = get_reason_from_key(key);
		if (reason != invalid_reason)
		{
			//This works out the direction of navigation.
			//First, Down and Right moves forward in the control order.
			//Last, Left and Up moves backward in the control order.
			const bool previous = ((reason == muxh::XamlSourceFocusNavigationReason::First) || (reason == muxh::XamlSourceFocusNavigationReason::Down) || (reason == muxh::XamlSourceFocusNavigationReason::Right)) ? false : true;
			//Obtains the currently focused window.
			const auto current_focused_window = GetFocus();
			//Uses the GetNextDlgTabItem to get focus next.
			//This basically searches for the next control with the WS_TABSTOP style.
			//This is where we use the direction calculated above. We want to work out
			//whether we want to get the previous or next window.
			const auto next_element = GetNextDlgTabItem(get_handle(), current_focused_window, previous);
			//Go through the cached xaml source objects.
			//If the window handle we want to change focus to matches one of the xaml sources window handles
			//then that is the xaml source that we wish to navigate to.
			for (auto &xaml_source : m_xaml_sources)
			{
				HWND island_window{};
				winrt::check_hresult(xaml_source.as<IDesktopWindowXamlSourceNative>()->get_WindowHandle(&island_window));
				if (next_element == island_window)
				{
					return xaml_source;
				}
			}
		}
	}

	return nullptr;
}

//Get the xaml source, if any, that has focus.
muxh::DesktopWindowXamlSource window_base::get_focused_island()
{
	for (auto &xaml_source : m_xaml_sources)
	{
		if (xaml_source.HasFocus())
		{
			return xaml_source;
		}
	}

	return nullptr;
}

//Moves focus between controls.
bool window_base::navigate_focus(MSG *msg)
{
	//The following is equivalent to:
	//const auto next_focused_island = get_next_focused_island(msg);
	//if(next_focused_island)
	//In otherwords if get_focused_island returns anything besides nullptr, the if case executes,
	//otherwise the else case executes.
	//This is important to navigate properly to a xaml island window.
	if (const auto next_focused_island = get_next_focused_island(msg))
	{
		//Gets the previous/currently focused window.
		const auto previous_focused_window = GetFocus();
		//Obtains the current window rectangle.
		//This is used for the hint rectangle.
		RECT rc_prev{};
		THROW_IF_WIN32_BOOL_FALSE(GetWindowRect(previous_focused_window, &rc_prev));
		HWND island_window = get_handle(next_focused_island);
		
		//Generates the XamlSourceFocusNavigationRequest object.
		POINT pt = { rc_prev.left, rc_prev.top };
		SIZE sz = { rc_prev.right - rc_prev.left, rc_prev.bottom - rc_prev.top };
		ScreenToClient(island_window, &pt);
		const auto hint_rect = wf::Rect({static_cast<float>(pt.x), static_cast<float>(pt.y), static_cast<float>(sz.cx), static_cast<float>(sz.cy)});
		const auto reason = get_reason_from_key(msg->wParam);
		const auto request = muxh::XamlSourceFocusNavigationRequest(reason, hint_rect);
		//Store the correlation id for the current request.
		m_last_focus_request_id = request.CorrelationId();
		//Attempt to navigate focus.
		const auto result = next_focused_island.NavigateFocus(request);
		auto focus_moved = result.WasFocusMoved();
		//I do not know if it is a bug, but NavigateFocus doesn't move the keyboard focus
		//for the associated HWND.
		//If WasFocusMoved indicates that the focus was moved, call SetFocus on the HWND itself
		//to move the keyboard focus.
		//Without this, the xaml content doesn't receive keyboard focus when pressing tab.
		if (focus_moved)
		{
			SetFocus(island_window);
		}
		return focus_moved;
	}
	else
	{
		//Figures out if an island has keyboard focus.
		const bool island_is_focused = get_focused_island() != nullptr;
		//Gets the state of the Alt key.
		byte keyboard_state[256] = {};
		THROW_IF_WIN32_BOOL_FALSE(GetKeyboardState(keyboard_state));
		const bool is_menu_modifier = (keyboard_state[VK_MENU] & 0x80);
		//If a xaml island has focus then we ignore any message that comes through here
		//unless the Alt key is being held. This allows us to access Windows API menu bars.
		if (island_is_focused && !is_menu_modifier)
		{
			return false;
		}
		//Passes the message through to IsDialogMessage.
		//If the xaml source isn't handling the message, this function implements
		//the Windows API dialog keyboard navigation. It handles Tab, Shift+Tab,
		//Left, Right, Up and Down cursor messages and more.
		//While the function has Dialog in the name, it is documented to work
		//on regular windows, not just dialog boxes.
		const bool is_dialog_message = !!IsDialogMessageW(get_handle(), msg);
		//The message pump and TakeFocusRequested events need to know if IsDialogMessage
		//handled the message.
		return is_dialog_message;
	}
}

//This event should fire when you navigate into a xaml source.
//For some reason it currently doesn't.
void window_base::on_got_focus(muxh::DesktopWindowXamlSource const &sender, muxh::DesktopWindowXamlSourceGotFocusEventArgs const &)
{
	MessageBoxW(get_handle(), L"GotFocusEvent", L"GotFocusEvent", MB_OK);
	HWND island_handle = get_handle(sender);

	if (GetFocus() != island_handle)
	{
		OutputDebugStringW(L"GotFocus fired, HWND does not have focus.");
	}
}

//This event fires when you navigate out of a xaml source. I.e, if a xaml source has focus and you press tab.
void window_base::on_take_focus_requested(muxh::DesktopWindowXamlSource const & sender, muxh::DesktopWindowXamlSourceTakeFocusRequestedEventArgs const &args)
{
	HWND sender_handle = get_handle(sender);

	//If the correlation id isn't the same as the last one, this means that we are 
	//navigating to a new window. 
	if (args.Request().CorrelationId() != m_last_focus_request_id)
	{
		const auto reason = args.Request().Reason();
		const bool previous = ((reason == muxh::XamlSourceFocusNavigationReason::First) || (reason == muxh::XamlSourceFocusNavigationReason::Down) || (reason == muxh::XamlSourceFocusNavigationReason::Right)) ? false : true;

		//Synthesise a MSG to navigate.
		MSG msg{};
		msg.hwnd = sender_handle;
		msg.message = WM_KEYDOWN;
		msg.wParam = get_key_from_reason(reason);
		//Try to move focus.
		if (!navigate_focus(&msg))
		{
			//If the navigate focus fails, get the next window with the WS_TABSTOP style
			//and set the focus on this window.
			const auto next_element = GetNextDlgTabItem(get_handle(), sender_handle, previous);
			SetFocus(next_element);
		}
	}
	else
	{
		const auto request = muxh::XamlSourceFocusNavigationRequest(muxh::XamlSourceFocusNavigationReason::Restore);
		m_last_focus_request_id = request.CorrelationId();
		sender.NavigateFocus(request);
		SetFocus(sender_handle);
	}
}

bool window_base::focus_navigate(MSG *msg)
{
	return navigate_focus(msg);
}

//Retrieves all of the created xaml sources for this window.
std::vector<muxh::DesktopWindowXamlSource> window_base::get_xaml_sources()
{
	std::vector<muxh::DesktopWindowXamlSource> sources;

	for (auto &xaml_source : m_xaml_sources)
	{
		sources.push_back(xaml_source);
	}

	return sources;
}

//Creates a DesktopWindowXamlSource out of a xaml UIElement.
//This function creates the object, hooks up the events, caches the source for later use
//and returns the handle to the containing window.
HWND window_base::create_desktop_window_xaml_source(DWORD extra_styles, const mux::UIElement &content)
{
	muxh::DesktopWindowXamlSource desktop_source;
	//Obtains the handle for the source and attaches this source to the main window.
	HWND xaml_source_handle = get_handle_and_attach(desktop_source, get_handle());
	_ASSERTE(xaml_source_handle != nullptr);
	//Add the style provided by the caller.
	//To do this, we get the styles from the xaml source handle, bitwise ors the provided styles
	//and then sets it on the xaml source handle.
	const DWORD ex_style = static_cast<DWORD>(GetWindowLongPtrW(xaml_source_handle, GWL_STYLE)) | extra_styles;
	SetWindowLongPtrW(xaml_source_handle, GWL_STYLE, static_cast<LONG_PTR>(ex_style));

	//Adds the provided content as content for the xaml source.
	desktop_source.Content(content);
	//Wires up the TakeFocusRequested event to the xaml source and stores the event token.
	m_take_focus_event_tokens.emplace(std::make_pair(xaml_source_handle, desktop_source.TakeFocusRequested({ this, &window_base::on_take_focus_requested })));
	//Wires up the GotFocus event to the xaml source and stores the event token.
	m_got_focus_event_tokens.emplace(std::make_pair(xaml_source_handle, desktop_source.GotFocus({ this, &window_base::on_got_focus })));
	//Stores the xaml source.
	m_xaml_sources.push_back(desktop_source);

	return xaml_source_handle;
}

//Unhooks the events and clears the xaml sources.
void window_base::clear_xaml_islands()
{
	for (auto &xaml_source : m_xaml_sources)
	{
		HWND xaml_source_handle = get_handle(xaml_source);
		auto take_focus_it = m_take_focus_event_tokens.find(xaml_source_handle);
		_ASSERTE(take_focus_it != m_take_focus_event_tokens.end());
		xaml_source.TakeFocusRequested((*take_focus_it).second);
		auto got_focus_it = m_got_focus_event_tokens.find(xaml_source_handle);
		_ASSERTE(got_focus_it != m_got_focus_event_tokens.end());
		xaml_source.GotFocus((*got_focus_it).second);
		xaml_source.Close();
	}
	m_take_focus_event_tokens.clear();
	m_got_focus_event_tokens.clear();
	m_xaml_sources.clear();
}

//Helper function that just obtains the window handle from the xaml source.
HWND window_base::get_handle(muxh::DesktopWindowXamlSource const &source)
{
	HWND island_handle{};

	winrt::check_hresult(source.as<IDesktopWindowXamlSourceNative>()->get_WindowHandle(&island_handle));
	return island_handle;
}

//Helper function that obtains the window handle from the xaml source, it also
//attaches the xaml source to a parent window.
HWND window_base::get_handle_and_attach(muxh::DesktopWindowXamlSource const &source, HWND attach_to)
{
	HWND island_handle{};
	auto interop = source.as<IDesktopWindowXamlSourceNative>();

	//This order matters. If you attempt to get the handle before the source
	//has been attached to a window, get_WindowHandle will receive a null
	//handle.
	winrt::check_hresult(interop->AttachToWindow(attach_to));
	winrt::check_hresult(interop->get_WindowHandle(&island_handle));

	return island_handle;
}

//Loads a xaml file from a disk file.
mux::UIElement LoadControlFromFile(std::wstring const &file_name)
{
	std::filesystem::path file_path = file_name;
	if (!std::filesystem::exists(file_path))
	{
		THROW_HR(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
	}

	auto file_size = std::filesystem::file_size(file_path);

	wil::unique_file xaml_control;
	std::wstring file_content;

	auto result = _wfopen_s(xaml_control.addressof(), file_path.c_str(), L"rb");
	if (result != 0)
	{
		THROW_HR(E_FAIL);
	}
	
	while (!feof(xaml_control.get()))
	{
		int val = fgetc(xaml_control.get());
		if (val != EOF)
		{
			file_content += static_cast<wchar_t>(val);
		}
	}

	if (file_content.size() < file_size)
	{
		THROW_HR(E_FAIL);
	}

	return muxm::XamlReader::Load(static_cast<winrt::hstring>(file_content)).as<mux::UIElement>();
}

//Loads a xaml file from an embedded resource.
//The resource must be type 255.
mux::UIElement LoadControlFromResource(uint16_t id)
{
	auto resource_handle = FindResourceW(nullptr, MAKEINTRESOURCEW(id), MAKEINTRESOURCEW(xamlresourcetype));
	THROW_LAST_ERROR_IF(!resource_handle);

	HGLOBAL resource_data = LoadResource(nullptr, resource_handle);
	THROW_LAST_ERROR_IF(!resource_data);

	auto data = static_cast<char *>(LockResource(resource_data));
	auto hstr_data = winrt::to_hstring(data);
	return muxm::XamlReader::Load(hstr_data).as<mux::UIElement>();
}