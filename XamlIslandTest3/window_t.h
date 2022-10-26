#pragma once

#include "window_base.h"

//Base class for our window.
//It implements the base message handling functionality.
//This uses CRTP to call the most derived message handler.
template <typename T>
class window_t : public window_base
{
public:
	using my_type = window_t<T>;
	using my_t = T;

protected:
	//WM_DESTROY handler.
	//Since this window is only going to be the primary top level window, it will
	//just post the WM_QUIT message.
	//This would have to be modified for multiple top level windows.
	void on_destroy()
	{
		PostQuitMessage(0);
	}

	//WM_ACTIVATE handler.
	//This handler just stores what window has keyboard
	//focus when we deactivate the window. This allows the window
	//to restore the focus correctly when the window regains
	//focus.
	void on_activate(uint16_t state, HWND, uint16_t)
	{
		if (state == WA_INACTIVE)
		{
			m_window_focus = GetFocus();
		}
	}

	//WM_SETFOCUS handler.
	//This handler returns focus to the window that had keyboard
	//focus when the window lost focus.
	void on_setfocus(HWND)
	{
		if (m_window_focus != nullptr)
		{
			SetFocus(m_window_focus);
		}
	}

	//The class' message handler.
	LRESULT handle_message(UINT msg, WPARAM wparam, LPARAM lparam)
	{
		switch (msg)
		{
		case WM_DESTROY:
		{
			on_destroy();
			return 0;
		}
		case WM_ACTIVATE:
		{
			on_activate(LOWORD(wparam), reinterpret_cast<HWND>(lparam), HIWORD(wparam));
			return 0;
		}
		case WM_SETFOCUS:
		{
			on_setfocus(reinterpret_cast<HWND>(wparam));
			return 0;
		}
		case WM_USER_QUERY_WINDOWBASE:
		{
			//This handles the WM_USER_QUERY_WINDOWBASE user message.
			//This is used to identify this window as a window derived from window_base.
			return query_window_base_identified;
		}
		case WM_USER_GET_WINDOWBASE_POINTER:
		{
			//This handles the WM_USER_GET_WINDOWBASE_POINTER user message.
			//This retrieves the window_base pointer for this class.
			return reinterpret_cast<LRESULT>(static_cast<window_base *>(this));
		}
		case WM_USER_VERIFY_POINTER:
		{
			//This handles the WM_USER_VERIFY_POINTER user message.
			//This is used to check the pointer provided by the user to determine
			//if it is the one that backs this window handle.
			window_base *ptr = reinterpret_cast<window_base *>(lparam);
			return ptr == static_cast<window_base *>(this) ? verify_window_base_pointer_match : verify_window_base_pointer_no_match;
		}
		}
		return DefWindowProcW(get_handle(), msg, wparam, lparam);
	}

	//The window procedure registered for the window handle.
	static LRESULT CALLBACK window_proc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		//The message handler for messages not WM_NCCREATE.
		if (msg != WM_NCCREATE)
		{
			//Obtains the window pointer from the window handle.
			//If this pointer isn't set, either because WM_NCCREATE hasn't been processed yet
			//or WM_NCDESTROY has been processed, then we just make sure that the message
			//goes to DefWindowProc.
			my_t *that = instance_from_handle(wnd);
			if (that)
			{
				if (msg != WM_NCDESTROY)
				{
					//If the message isn't WM_NCDESTROY then we just pass the message to the
					//class' message handler.
					return that->handle_message(msg, wparam, lparam);
				}
				else
				{
					//If the message is WM_NCDESTROY then we pass the message to the class'
					//message handler first, then do extra processing.
					auto result = that->handle_message(msg, wparam, lparam);
					process_ncdestroy(wnd);
					return result;
				}
			}
		}
		else
		{
			//The message handler for WM_NCCREATE.
			if (!process_nccreate(wnd, *reinterpret_cast<CREATESTRUCTW *>(lparam)))
			{
				return FALSE;
			}
			my_t *that = instance_from_handle(wnd);
			return that->handle_message(msg, wparam, lparam);
		}

		return DefWindowProcW(wnd, msg, wparam, lparam);
	}

	//Helper function that, given a HWND, retrieves the pointer to the window class.
	//We can assume that this will be the correct type since this will only be
	//set initially from the most derived class.
	//The only issue is that this uses the GWLP_USERDATA area for this information.
	static my_t *instance_from_handle(HWND wnd)
	{
		my_t *ptr = reinterpret_cast<my_t *>(GetWindowLongPtrW(wnd, GWLP_USERDATA));
		return ptr;
	}

	//Does the processing for the WM_NCCREATE message.
	//This sets the pointer to the class in the HWND's user data and then sets the HWND
	//to the class.
	static bool process_nccreate(HWND wnd, const CREATESTRUCTW &cs)
	{
		//SetWindowLongPtr doesn't reset the last error. This is used to make sure that
		//SetWindowLongPtr succeeds.
		SetLastError(ERROR_SUCCESS);
		//The lpCreateParams corresponds to the lpParam parameter of CreateWindowEx.
		//This is how we passed in the pointer to the window class.
		//We use this to associate the HWND with the class.
		auto result = SetWindowLongPtrW(wnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs.lpCreateParams));
		_ASSERTE(result == 0);
		DWORD last_error = GetLastError();
		if (last_error != ERROR_SUCCESS)
		{
			THROW_WIN32(last_error);
		}

		//Now that we have the HWND and pointer connected, the reverse association needs to occur.
		//This sets the handle in the window class.
		my_t *ptr = instance_from_handle(wnd);
		ptr->set_handle(wnd);

		return true;
	}
	static void process_ncdestroy(HWND wnd)
	{
		SetWindowLongPtrW(wnd, GWLP_USERDATA, 0);
	}

private:
	HWND m_window_focus = nullptr;
};

template<typename T>
auto LoadControlFromFile(std::wstring const &file_name) -> T
{
	return LoadControlFromFile(file_name).as<T>();
}
template<typename T>
auto LoadControlFromResource(uint16_t id) -> T
{
	return LoadControlFromResource(id).as<T>();
}