#include "lilith_is_all_you_need.h"

//int bitwise 说明如下
//至少16位，msvc下32位
//00000000000000000000000000000000
//      zyxwvutsrqponmlkjihgfedcba
//a:控制是不是开始了capture



enum {
	f_a = 0, // 捕获
	f_b = 1, // 是否是类名
	f_c = 2, // 是否是白名单模式
	f_d = 3, // 是否是透明模式
	f_e = 4, // 是否开启老板键
	f_f = 5, // 是否已经注册热键
	f_g = 6, // 是否开启保存
	f_h = 7
};

LRESULT CALLBACK pic_subclass_proc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam){
	struct context* ptr_ctx = (struct context*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if(ptr_ctx == NULL){
		return DefWindowProcW(hwnd, umsg, wparam, lparam);
	}
	switch (umsg)
	{
		case WM_LBUTTONDOWN: {
			SetCapture(hwnd);
			CLEAR_BIT(ptr_ctx->panel_ctx.bitwise, f_b);
			SendMessageW(hwnd, STM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)ptr_ctx->panel_ctx.using_ico);
			SET_BIT(ptr_ctx->panel_ctx.bitwise, f_a);
			break;
		}
		case WM_MOUSEMOVE: {
			if (CHECK_BIT(ptr_ctx->panel_ctx.bitwise,f_a) == 1) {
				SetCursor(ptr_ctx->panel_ctx.using_cur);
				POINT curr_pt;
				GetCursorPos(&curr_pt);
				HWND target_hwnd = WindowFromPoint(curr_pt);
				if (target_hwnd != NULL) {
					// 强制追溯顶级窗口
					target_hwnd = GetAncestor(target_hwnd, GA_ROOT);
				}
				
				if (target_hwnd != ptr_ctx->panel_ctx.target_window_hwnd &&
					target_hwnd != hwnd &&
					target_hwnd != GetParent(hwnd) && target_hwnd != NULL)
				{
					ptr_ctx->panel_ctx.target_window_hwnd = target_hwnd;
					WCHAR window_text[256] = { 0 };
					GetWindowTextW(target_hwnd, window_text, 256);
					if (window_text[0] == L'\0') {
						SET_BIT(ptr_ctx->panel_ctx.bitwise, f_b);
						GetClassNameW(target_hwnd, window_text, 256);
					}
					else {
						CLEAR_BIT(ptr_ctx->panel_ctx.bitwise, f_b);
					}
					SetDlgItemTextW(GetParent(hwnd), IDC_WHITELIST_INPUT_BOX, window_text);
				}
			}
			break;
		}
		case WM_LBUTTONUP: {
			if (CHECK_BIT(ptr_ctx->panel_ctx.bitwise, f_a) == 1) {
				SetCursor(ptr_ctx->panel_ctx.normal_cur);
				ReleaseCapture();
				SendMessageW(hwnd, STM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)ptr_ctx->panel_ctx.pending_ico);
				CLEAR_BIT(ptr_ctx->panel_ctx.bitwise, f_a);
			}
			break;
		}
		case WM_CAPTURECHANGED: {
			if (CHECK_BIT(ptr_ctx->panel_ctx.bitwise, f_a) == 1) {
				SetCursor(ptr_ctx->panel_ctx.normal_cur);
				SendMessageW(hwnd, STM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)ptr_ctx->panel_ctx.pending_ico);
				CLEAR_BIT(ptr_ctx->panel_ctx.bitwise, f_a);
			}
			break;
		}
	}
	if(hwnd != 0)
		return CallWindowProcW(ptr_ctx->panel_ctx.prev_picture_box_proc, hwnd, umsg, wparam, lparam);
	return 0;
}

DWORD WINAPI apply_inject_worker_thread(LPVOID lpParam) {
	struct inject_task* task = (struct inject_task*)lpParam;
	if (task == NULL) return 0;

	if (task->remove_global) {
		EnumWindows(enum_windows_proc_remove, (LPARAM)task->main_ctx);
	}
	else {
		for (int i = 0; i < task->remove_count; i++) {
			if (task->hwnd_to_remove[i] != NULL) {
				remove_anti_capture(task->hwnd_to_remove[i], task->main_ctx);
			}
		}
	}
	if (task->is_global_mode) {
		EnumWindows(enum_windows_proc_add, (LPARAM)task->main_ctx);
	}
	else {
		for (int i = 0; i < task->add_count; i++) {
			if (task->hwnd_to_add[i] != NULL) {
				apply_anti_capture(task->hwnd_to_add[i], task->main_ctx);
			}
		}
	}
	PostMessageW(task->main_ctx->panel_ctx.panel_hwnd, WM_INJECT_FINISHED, 0, 0);
	free(task);
	return 0;
}

INT_PTR CALLBACK dlg_proc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) {
	switch (umsg) {
	case WM_INITDIALOG: {
		//初始化
		struct context* ptr_ctx = (struct context*)lparam;
		if (ptr_ctx != NULL) {
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)ptr_ctx);
			HMODULE curr_module = GetModuleHandleW(NULL);
			if (curr_module == NULL) {
				return (INT_PTR)FALSE;
			}
			ptr_ctx->panel_ctx.pending_ico = LoadIconW(curr_module, MAKEINTRESOURCEW(IDI_ICON_PENDING));
			ptr_ctx->panel_ctx.using_ico = LoadIconW(curr_module, MAKEINTRESOURCEW(IDI_ICON_USING));
			ptr_ctx->panel_ctx.normal_cur = LoadCursorW(NULL, MAKEINTRESOURCEW(IDC_ARROW));
			ptr_ctx->panel_ctx.using_cur = LoadCursorW(curr_module, MAKEINTRESOURCEW(IDC_CURSOR_USING));
			if (ptr_ctx->panel_ctx.pending_ico == NULL || ptr_ctx->panel_ctx.using_ico == NULL) {
				return (INT_PTR)FALSE;
			}
			if (ptr_ctx->panel_ctx.normal_cur == NULL || ptr_ctx->panel_ctx.using_cur == NULL) {
				return (INT_PTR)FALSE;
			}
			HWND picture_box_handle = GetDlgItem(hwnd, IDC_FIND_BOX);
			if (picture_box_handle == NULL) {
				return (INT_PTR)FALSE;
			}
			CheckDlgButton(hwnd, IDC_WHITELIST_RAD, BST_CHECKED);
			CheckDlgButton(hwnd, IDC_TRANSPARENT_RAD, BST_CHECKED);
			SendDlgItemMessageW(hwnd, IDC_FIND_BOX, STM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)ptr_ctx->panel_ctx.pending_ico);
			SetWindowLongPtrW(picture_box_handle, GWLP_USERDATA, (struct context*)ptr_ctx);
			ptr_ctx->panel_ctx.prev_picture_box_proc = (WNDPROC)SetWindowLongPtr(picture_box_handle, GWLP_WNDPROC, (LONG_PTR)pic_subclass_proc);
		}
		return (INT_PTR)TRUE;
		break;
	}
	case WM_COMMAND: {
		struct context* ptr_ctx = (struct context*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (ptr_ctx == NULL) {
			return (INT_PTR)FALSE;
		}
		switch (LOWORD(wparam)) {
		case IDC_ADD_MANUAL: {
			
			if (IsWindow(ptr_ctx->panel_ctx.target_window_hwnd)) {
				wchar_t text_window[256] = { 0 };
				if (CHECK_BIT(ptr_ctx->panel_ctx.bitwise, f_b) == 1) { // 类名
					GetClassNameW(ptr_ctx->panel_ctx.target_window_hwnd, text_window, 256);
				}
				else {
					GetWindowTextW(ptr_ctx->panel_ctx.target_window_hwnd,text_window,256);
				}
				HWND list_box_handle = GetDlgItem(hwnd, IDC_WHITELIST_LIST);
				if (list_box_handle == NULL) {
					return (INT_PTR)FALSE;
				}
				bool is_exist = false;
				LRESULT cnt_elems = SendMessageW(list_box_handle, LB_GETCOUNT, 0, 0);
				for (int i = 0; i < cnt_elems; i++) {
					HWND curr_hwnd = (HWND)SendMessageW(list_box_handle, LB_GETITEMDATA, i, 0);
					if (ptr_ctx->panel_ctx.target_window_hwnd == curr_hwnd) {
						is_exist = true;
						break;
					}
				}
				if (is_exist == false) {
					LRESULT idx = SendMessageW(list_box_handle, LB_ADDSTRING, 0, (LPARAM)text_window);
					if (idx != LB_ERR) {
						SendMessageW(list_box_handle, LB_SETITEMDATA, idx, (LPARAM)ptr_ctx->panel_ctx.target_window_hwnd);
					}
				}
			}
			break;
		}
		case IDC_REMOVE_MANUAL: {
			HWND list_box_handle = GetDlgItem(hwnd, IDC_WHITELIST_LIST);
			if (list_box_handle == NULL) {
				return (INT_PTR)FALSE;
			}
			LRESULT sel_idx = SendMessageW(list_box_handle, LB_GETCURSEL, 0, 0);
			if (sel_idx != LB_ERR) {
				SendMessage(list_box_handle, LB_DELETESTRING, sel_idx, 0);
			}
			break;
		}
		case IDC_APPLY_CHANGE: {
			if (ptr_ctx == NULL) {
				return (INT_PTR)FALSE;
			}
			if (ptr_ctx->panel_ctx.is_boss_key_hidden) {
				ptr_ctx->panel_ctx.is_boss_key_hidden = false;
				if (CHECK_BIT(ptr_ctx->panel_ctx.bitwise, f_c) != 0) { // 如果旧状态是白名单
					for (int i = 0; i < 256; i++) {
						if (ptr_ctx->target[i] != NULL && IsWindow(ptr_ctx->target[i])) {
							ShowWindow(ptr_ctx->target[i], SW_SHOW);
						}
					}
				}
			}
			bool old_was_global = (CHECK_BIT(ptr_ctx->panel_ctx.bitwise, f_c) == 0);
			if (IsDlgButtonChecked(hwnd, IDC_WHITELIST_RAD) == BST_CHECKED) {
				SET_BIT(ptr_ctx->panel_ctx.bitwise, f_c); // 白名单
			}
			else {
				CLEAR_BIT(ptr_ctx->panel_ctx.bitwise, f_c);
			}
			if (IsDlgButtonChecked(hwnd, IDC_TRANSPARENT_RAD) == BST_CHECKED) {
				SET_BIT(ptr_ctx->panel_ctx.bitwise, f_d); // 穿透
			}
			else {
				CLEAR_BIT(ptr_ctx->panel_ctx.bitwise, f_d);
			}
			if (IsDlgButtonChecked(hwnd, IDC_ENABLE_BOSS_KEY) == BST_CHECKED) {
				SET_BIT(ptr_ctx->panel_ctx.bitwise, f_e); // 老板键
				//处理热键
				HWND hwnd_hot_key = GetDlgItem(hwnd, IDC_HOTKEY_BOSS);
				if (hwnd_hot_key == NULL) {
					return FALSE;
				}
				LRESULT result = SendMessageW(hwnd_hot_key, HKM_GETHOTKEY, 0, 0);
				ptr_ctx->panel_ctx.hotkey = LOWORD(result); // 低16
				ptr_ctx->panel_ctx.virtual_code = LOBYTE(result); // 虚拟键码
				ptr_ctx->panel_ctx.modifiers = HIBYTE(result); // 修饰
				if (ptr_ctx->panel_ctx.virtual_code == VK_F12) {
					if (ptr_ctx->panel_ctx.hotkey == 0 || ptr_ctx->panel_ctx.modifiers == HOTKEYF_SHIFT) {
						MessageBox(hwnd, L"F12和Shift+F12系统保留，无法注册", L"NoMoreCapture", MB_SYSTEMMODAL | MB_ICONWARNING);
						return FALSE; // 不往下执行逻辑
					}
					break;
				}
				if (ptr_ctx->panel_ctx.virtual_code == 0 && ptr_ctx->panel_ctx.modifiers == 0) {
					MessageBox(hwnd, L"没有设置热键，默认关闭老板键", L"NoMoreCapture", MB_SYSTEMMODAL | MB_ICONWARNING);
					CLEAR_BIT(ptr_ctx->panel_ctx.bitwise, f_e);
					if (CHECK_BIT(ptr_ctx->panel_ctx.bitwise, f_f) == 1) { // 之前设置了
						CLEAR_BIT(ptr_ctx->panel_ctx.bitwise, f_f);
						UnregisterHotKey(ptr_ctx->window_hwnd, HOTKEY_ID);
					}
				}
				BYTE mod = ptr_ctx->panel_ctx.modifiers;
				UINT mod_reg = 0;
				if (mod & HOTKEYF_ALT) {
					mod_reg |= MOD_ALT;
				}
				if (mod & HOTKEYF_CONTROL) {
					mod_reg |= MOD_CONTROL;
				}
				if (mod & HOTKEYF_SHIFT) {
					mod_reg |= MOD_SHIFT;
				}
				if (CHECK_BIT(ptr_ctx->panel_ctx.bitwise, f_f) == 1){ // 先卸载
					CLEAR_BIT(ptr_ctx->panel_ctx.bitwise, f_f);
					UnregisterHotKey(ptr_ctx->window_hwnd, HOTKEY_ID);
				}
				if (RegisterHotKey(ptr_ctx->window_hwnd, HOTKEY_ID, mod_reg, ptr_ctx->panel_ctx.virtual_code)) {
					SET_BIT(ptr_ctx->panel_ctx.bitwise, f_f); // 设置了热键
				}
				ptr_ctx->panel_ctx.mod_reg = mod_reg;
			}
			else {
				CLEAR_BIT(ptr_ctx->panel_ctx.bitwise, f_e);
			}

			if (IsDlgButtonChecked(hwnd, IDC_SAVE_CFG) == BST_CHECKED) {
				SET_BIT(ptr_ctx->panel_ctx.bitwise, f_h); // 开启保存
			}
			else {
				CLEAR_BIT(ptr_ctx->panel_ctx.bitwise, f_h);
			}
			HWND dlg_hwnd = ptr_ctx->panel_ctx.panel_hwnd;
			EnableWindow(GetDlgItem(hwnd, IDC_APPLY_CHANGE), FALSE);
			EnableWindow(GetDlgItem(hwnd, IDC_ADD_MANUAL), FALSE);
			EnableWindow(GetDlgItem(hwnd, IDC_REMOVE_MANUAL), FALSE);
			HWND list_box_handle = GetDlgItem(dlg_hwnd, IDC_WHITELIST_LIST);
			if (list_box_handle == NULL) {
				return (INT_PTR)FALSE;
			}
			bool is_exist = false;
			LRESULT cnt_elems = SendMessageW(list_box_handle, LB_GETCOUNT, 0, 0);
			if (cnt_elems > 128) {
				MessageBoxW(hwnd, L"超过了白名单最大上限（128)", L"NoMoreCapture", MB_SYSTEMMODAL | MB_ICONWARNING);
				return (INT_PTR)FALSE;
			}
			//for (int i = 0; i < 128; i	++) {
			//	if (ptr_ctx->target[i] != NULL) {
			//		remove_anti_capture(ptr_ctx->target[i],ptr_ctx);
			//	}
			//}
			
			// 动态分配任务结构体（防止线程跑的时候栈内存失效）
			struct inject_task* task = (struct inject_task*)malloc(sizeof(struct inject_task));
			if (task == 0) {
				return (INT_PTR)FALSE;
			}
			memset(task, 0, sizeof(struct inject_task));
			task->main_ctx = ptr_ctx;
			if (old_was_global) {
				task->remove_global = true; // 告诉线程：去全局卸载
			}
			else {
				task->remove_global = false;
				for (int i = 0; i < 128; i++) {
					if (ptr_ctx->target[i] != NULL) {
						task->hwnd_to_remove[task->remove_count++] = ptr_ctx->target[i];
					}
				}
			}
			// 清空现有的 target 记录
			memset(ptr_ctx->target, 0, sizeof(ptr_ctx->target));

			if (CHECK_BIT(ptr_ctx->panel_ctx.bitwise, f_c) != 0) {
				// 白名单模式
				task->is_global_mode = false;
				for (int i = 0; i < cnt_elems; i++) {
					HWND curr_hwnd = (HWND)SendMessageW(list_box_handle, LB_GETITEMDATA, i, 0);
					ptr_ctx->target[i] = curr_hwnd; // 存入上下文以便后续追踪
					task->hwnd_to_add[task->add_count++] = curr_hwnd;
				}
			}
			else {
				// 全局模式
				task->is_global_mode = true;
			}

			// 启动后台线程执行注入
			HANDLE hThread = CreateThread(NULL, 0, apply_inject_worker_thread, task, 0, NULL);
			if (hThread) {
				CloseHandle(hThread);
			}
			else {
				free(task);
				EnableWindow(GetDlgItem(hwnd, IDC_APPLY_CHANGE), TRUE);
			}
			break;
		}
		}
		return (INT_PTR)TRUE;
	}
	case WM_INJECT_FINISHED: {
		EnableWindow(GetDlgItem(hwnd, IDC_APPLY_CHANGE), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_ADD_MANUAL), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_REMOVE_MANUAL), TRUE);
		return (INT_PTR)TRUE;
	}
	case WM_CLOSE: {
		ShowWindow(hwnd, SW_HIDE);
		return (INT_PTR)TRUE; // 说明处理完了
	}
	case WM_DESTROY: {
		struct context* ptr_ctx = (struct context*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
		if (ptr_ctx != NULL) {
			if (CHECK_BIT(ptr_ctx->panel_ctx.bitwise, f_c) != 0) {
				// 白名单模式清理
				for (int i = 0; i < 128; i++) {
					if (ptr_ctx->target[i] != NULL) {
						remove_anti_capture(ptr_ctx->target[i], ptr_ctx);
					}
				}
			}
			else {
				// 全局模式清理
				EnumWindows(enum_windows_proc_remove, (LPARAM)ptr_ctx);
			}
			if (CHECK_BIT(ptr_ctx->panel_ctx.bitwise, f_f) == 1) {
				UnregisterHotKey(hwnd, HOTKEY_ID);
			}
		}
		PostQuitMessage(0);
		break;
	}
	}
	return (INT_PTR)FALSE;
}