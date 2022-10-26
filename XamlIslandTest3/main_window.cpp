#include "pch.h"
#include "main_window.h"
#include "resource.h"

namespace wf = winrt::Windows::Foundation;
namespace mux = winrt::Microsoft::UI::Xaml;
namespace muxx = winrt::Microsoft::UI::Xaml::XamlTypeInfo;
namespace muxc = winrt::Microsoft::UI::Xaml::Controls;

main_window::main_window(HINSTANCE inst) : m_instance(inst)
{
}

//Registers a window class and then create a window based on this class.
bool main_window::create_window(int cmdshow)
{
	register_window_class();
	DWORD styles = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME;

	HWND window_handle = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW, my_type::window_class, L"Test Window", styles, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, m_instance, this);
	if (window_handle == nullptr)
	{
		return false;
	}

	ShowWindow(window_handle, cmdshow);
	UpdateWindow(window_handle);
	SetFocus(window_handle);

	return true;
}

//The most derived message handler.
LRESULT main_window::handle_message(UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_NCCREATE:
	{
		//If we process WM_NCCREATE, we must still pass this through to
		//DefWindowProc. Failure to do this results in the window not showing properly.
		if (!on_nccreate(*reinterpret_cast<CREATESTRUCTW *>(lparam)))
		{
			return FALSE;
		}
		DefWindowProcW(get_handle(), msg, wparam, lparam);
		return TRUE;
	}
	case WM_CREATE:
	{
		if (!on_create(*reinterpret_cast<CREATESTRUCTW *>(lparam)))
		{
			return -1;
		}
		return 0;
	}
	case WM_DESTROY:
	{
		on_destroy();
		return 0;
	}
	case WM_SIZE:
	{
		on_size(static_cast<UINT>(wparam), LOWORD(lparam), HIWORD(lparam));
		return 0;
	}
	default:
	{
		//Any message that we don't handle gets passed to the base message handler.
		return my_base::handle_message(msg, wparam, lparam);
	}
	}
	//return my_base::handle_message(msg, wparam, lparam);
}

bool main_window::on_nccreate(const CREATESTRUCTW &)
{
	//The application is marked as high dpi aware.
	//Use this to initialise the high dpi information for the window.
	initialise_dpi();

	return true;
}
bool main_window::on_create(const CREATESTRUCTW &)
{
	//Creates a Windows API button.
	//This button is to illustrate the control navigation.
	m_native_button1.reset(CreateWindowExW(0, L"Button", L"Test Button 1", WS_TABSTOP | WS_CHILD | BS_PUSHBUTTON | BS_NOTIFY | WS_VISIBLE, 0, 0, 150, 50, get_handle(), reinterpret_cast<HMENU>(101), m_instance, nullptr));

	//Loads in a xaml control.
	m_xaml_button = LoadControlFromResource<muxc::Button>(IDR_XAML_CONTROL);
	m_xaml_button.Height(50);
	m_xaml_button.Width(100);
	m_xaml_button.HorizontalAlignment(mux::HorizontalAlignment::Left);
	m_xaml_button.VerticalAlignment(mux::VerticalAlignment::Top);

	//Creates the xaml source from the control loaded.
	m_xaml_button_handle = create_desktop_window_xaml_source(WS_TABSTOP, m_xaml_button);
	//Hooks up the Click event to the xaml button.
	m_xaml_button_click_revoker = m_xaml_button.Click(winrt::auto_revoke, [](wf::IInspectable const &sender, mux::RoutedEventArgs const &) {
		static int click_count = 0;
		++click_count;
		auto s = std::format(L"Click Count: {}", click_count);
		sender.as<muxc::Button>().Content(winrt::box_value(s.c_str()));
		});
	//Shows the xaml content window.
	ShowWindow(m_xaml_button_handle, SW_SHOW);

	//Creates another Windows API button.
	//This button is also used to illustrate the control navigation.
	m_native_button2.reset(CreateWindowExW(0, L"Button", L"Test Button 2", WS_TABSTOP | WS_CHILD | BS_PUSHBUTTON | BS_NOTIFY | WS_VISIBLE, 0, 0, 150, 50, get_handle(), reinterpret_cast<HMENU>(102), m_instance, nullptr));

	return true;
}
void main_window::on_destroy()
{
	//Clears and revokes all xaml content.
	clear_xaml_islands();
	m_xaml_button_click_revoker.revoke();
	m_xaml_button = nullptr;
	m_xaml_button_handle = nullptr;
	//This allows us to continue to perform the base class' handling of this
	//event, which in this case posts the WM_QUIT message.
	my_base::on_destroy();
}
void main_window::on_size(UINT, int, int)
{
	//Position the controls.
	//All sizes and positions are in virtual pixels, to agree with how xaml works.
	//This means that the pixels are scaled by the window DPI.
	if (m_native_button1)
	{
		//The button is 50 vpx tall and 150 vpx wide
		//Position it at 0,0

		SetWindowPos(m_native_button1.get(), nullptr, 0, 0, static_cast<int>(150 * m_window_dpi_scale), static_cast<int>(50 * m_window_dpi_scale), SWP_NOZORDER);
	}
	if (m_xaml_button_handle)
	{
		//The button is 50 vpx tall and 150 vpx wide
		//Position it at 160,0

		SetWindowPos(m_xaml_button_handle, nullptr, static_cast<int>(160 * m_window_dpi_scale), 0, static_cast<int>(160 * m_window_dpi_scale), static_cast<int>(60 * m_window_dpi_scale), SWP_NOZORDER);
		m_xaml_button.Width(150);
		m_xaml_button.Height(50);
	}
	if (m_native_button2)
	{
		//Use 50 vpx tall and 150 vpx wide
		//Position is at 320, 0

		SetWindowPos(m_native_button2.get(), nullptr, static_cast<int>(320 * m_window_dpi_scale), 0, static_cast<int>(150 * m_window_dpi_scale), static_cast<int>(50 * m_window_dpi_scale), SWP_NOZORDER);
	}
}

//Fills in the DPI information.
//This assumes that 96 DPI is the base/100% scale.
void main_window::initialise_dpi()
{
	m_window_dpi = GetDpiForWindow(get_handle());
	m_window_dpi_scale = m_window_dpi / 96.f;
}

//Window class registration functions.
bool main_window::check_class_registered()
{
	WNDCLASSEXW wcx{ sizeof(WNDCLASSEXW) };

	auto result = GetClassInfoExW(m_instance, my_type::window_class, &wcx);
	if (!result)
	{
		DWORD last_error = GetLastError();
		if (last_error == ERROR_CLASS_DOES_NOT_EXIST)
		{
			return false;
		}
		THROW_WIN32(last_error);
	}

	return true;
}
void main_window::register_window_class()
{
	if (check_class_registered())
	{
		return;
	}

	WNDCLASSEXW wcx{ sizeof(WNDCLASSEXW) };
	wcx.hInstance = m_instance;
	wcx.lpszClassName = my_type::window_class;
	wcx.lpfnWndProc = my_base::window_proc;
	wcx.style = CS_HREDRAW | CS_VREDRAW;

	wcx.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	wcx.hCursor = reinterpret_cast<HCURSOR>(LoadImageW(nullptr, MAKEINTRESOURCEW(OCR_NORMAL), IMAGE_CURSOR, 0, 0, LR_SHARED | LR_DEFAULTSIZE | LR_DEFAULTCOLOR));
	wcx.hIcon = reinterpret_cast<HICON>(LoadImageW(nullptr, MAKEINTRESOURCEW(OIC_SAMPLE), IMAGE_ICON, 0, 0, LR_SHARED | LR_DEFAULTSIZE | LR_DEFAULTCOLOR));
	wcx.hIconSm = reinterpret_cast<HICON>(LoadImageW(nullptr, MAKEINTRESOURCEW(OIC_SAMPLE), IMAGE_ICON, GetSystemMetricsForDpi(SM_CXSMICON, GetDpiForWindow(get_handle())), GetSystemMetricsForDpi(SM_CYSMICON, GetDpiForWindow(get_handle())), LR_SHARED | LR_DEFAULTCOLOR));

	THROW_IF_WIN32_BOOL_FALSE(static_cast<BOOL>(RegisterClassExW(&wcx)));
}