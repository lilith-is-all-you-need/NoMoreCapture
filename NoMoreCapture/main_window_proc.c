#include "context.h"
#include "lilith_is_all_you_need.h"

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

inline bool is_a_move_opt(int prev_x, int prev_y, int curr_x, int curr_y) {
    return (abs(prev_x - curr_x) > threshold) || (abs(prev_y - curr_y) > threshold);
}

void show_lilith_pop_menu(struct context* ptr_ctx,const POINT *pt_screen) {
    if (ptr_ctx == NULL || pt_screen == NULL) {
        return;
    }
    HMENU menu_handle = LoadMenuW(GetModuleHandleW(NULL), MAKEINTRESOURCE(IDR_LILITH_MENU));
    if (menu_handle != NULL) {
        HMENU sub_menu_handle = GetSubMenu(menu_handle, 0);
        if (sub_menu_handle != NULL) {
            SetForegroundWindow(ptr_ctx->window_hwnd);
            TrackPopupMenu(sub_menu_handle, TPM_RIGHTBUTTON | TPM_TOPALIGN | TPM_LEFTALIGN, pt_screen->x, pt_screen->y, 0, ptr_ctx->window_hwnd, NULL);
        }
        DestroyMenu(menu_handle);
    }
}

void show_tray_pop_menu(struct context *ptr_ctx) {
    POINT pt;
    GetCursorPos(&pt);
    HMENU menu_handle = LoadMenuW(GetModuleHandleW(NULL), MAKEINTRESOURCE(IDR_TRAY_MENU));
    if (menu_handle != NULL) {
        HMENU sub_menu_handle = GetSubMenu(menu_handle, 0);
        if (sub_menu_handle != NULL) {
            SetForegroundWindow(ptr_ctx->window_hwnd);
            TrackPopupMenu(sub_menu_handle, TPM_RIGHTBUTTON | TPM_TOPALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, ptr_ctx->window_hwnd, NULL);
            PostMessage(ptr_ctx->window_hwnd, WM_NULL, 0, 0); // 神秘windows
        }
    }
}

LRESULT CALLBACK lilith_wndproc(
    HWND hwnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam
) {
    switch (msg) {
    case WM_NCCALCSIZE:
        if (wparam == TRUE) {
            return 0;
        }
        break;
    case WM_NCCREATE: {
        CREATESTRUCT* ptr_cs = (CREATESTRUCT*)lparam;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)ptr_cs->lpCreateParams);
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
    case WM_NCHITTEST: {
        struct context* ptr_ctx = (struct context*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        if (ptr_ctx == NULL || ptr_ctx->bits == NULL) {
            return HTNOWHERE;
        }
        POINT pt = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
        ScreenToClient(hwnd, &pt);
        if (pt.x < 0 || pt.y < 0 || pt.x >= ptr_ctx->img_x || pt.y >= ptr_ctx->img_y)
            return HTNOWHERE;

        BYTE* p = (BYTE*)ptr_ctx->bits + (pt.y * ptr_ctx->window_width + pt.x) * 4; // pt.y * width + pt.x
        BYTE alpha = p[3];         // 读DIB该像素的 Alpha
        // return (p[3] < 10) ? HTTRANSPARENT : HTCAPTION;
        return (p[3] < 10) ? HTTRANSPARENT : HTCLIENT;
    }
    case WM_LBUTTONDOWN: {
        struct context* ptr_ctx = (struct context*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        if (ptr_ctx == NULL) {
            return HTNOWHERE;
        }
        ptr_ctx->pt_mouse_down.x = GET_X_LPARAM(lparam);
        ptr_ctx->pt_mouse_down.y = GET_Y_LPARAM(lparam);
        ptr_ctx->is_mouse_down = true;
        SetCapture(hwnd);
        return 0;
    }
    case WM_MOUSEMOVE: {
        struct context* ptr_ctx = (struct context*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        if (ptr_ctx == NULL) {
            return HTNOWHERE;
        }
        if (ptr_ctx->is_mouse_down == true) {
            int curr_x = GET_X_LPARAM(lparam);
            int curr_y = GET_Y_LPARAM(lparam);
            if (is_a_move_opt(ptr_ctx->pt_mouse_down.x, ptr_ctx->pt_mouse_down.y, curr_x, curr_y) == true) {
                ptr_ctx->is_mouse_down = false;
                ReleaseCapture();
                SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
            }
        }
        return 0;
    }
    case WM_LBUTTONUP: {
        struct context* ptr_ctx = (struct context*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        if (ptr_ctx == NULL) {
            return HTNOWHERE;
        }
        if (ptr_ctx->is_mouse_down == true) {
            ptr_ctx->is_mouse_down = false;
            ReleaseCapture();
            POINT pt_screen = ptr_ctx->pt_mouse_down;
            ClientToScreen(hwnd, &pt_screen);
            show_lilith_pop_menu(ptr_ctx, &pt_screen);
        }
        return 0;
    }
    case WM_RBUTTONUP: {
        struct context* ptr_ctx = (struct context*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        if (ptr_ctx == NULL) {
            return HTNOWHERE;
        }
        POINT pt_screen = { GET_X_LPARAM(lparam),GET_Y_LPARAM(lparam) };
        ClientToScreen(hwnd, &pt_screen);
        show_lilith_pop_menu(ptr_ctx, &pt_screen);
        return 0;
    }
    case WM_TRAYICON: {
        struct context* ptr_ctx = (struct context*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        if (wparam == 1001) { // 我滴图标
            switch (LOWORD(lparam)) {
            case WM_LBUTTONDBLCLK: {
                ShowWindow(ptr_ctx->window_hwnd, SW_SHOW);
                //双击
                break;
            }
            case WM_LBUTTONUP: {
                show_tray_pop_menu(ptr_ctx);
                //左键
                break;
            }
            case WM_RBUTTONUP: {
                show_tray_pop_menu(ptr_ctx);
                //右键
                break;
            }
            }
        }
        return 0;
    }
    case WM_COMMAND: {
        struct context* ptr_ctx = (struct context*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        int wm_id = LOWORD(wparam);
        switch (wm_id) {
        case ID_ABOUT: {
            MessageBoxW(hwnd, L"开发者 Bad_Dream_UwU", L"NoMoreCapture", MB_OK | MB_ICONINFORMATION);
            break;
        }
        case ID_HELP: {
            // TODO:加一个更精良的UI
            const wchar_t* help_text =
                L"【NoMoreCapture - 使用手册】\n\n"
                L"1.目标捕获\n"
                L"按住控制面板上的雷达图标拖动到目标窗口释放，即可自动识别窗口并添加到白名单列表中。\n\n"
                L"2.捕获模式设置\n"
                L"• 白名单模式：仅对列表内添加的窗口生效防截图/录屏。\n"
                L"• 全局模式：关闭白名单后，强制注入并隐藏除系统核心进程外的所有窗口。\n"
                L"• 穿透模式：开启时，截图录屏中该窗口会显示为黑色块(WDA_MONITOR)；关闭时，完全透明隐藏(WDA_EXCLUDEFROMCAPTURE)。\n\n"
                L"3.老板键功能\n"
                L"4.设置热键后（注意避开系统保留按键如F12），按下即可一键隐藏/显示目标窗口，同时隐藏 Lilith 软件自身界面。\n\n"
                L"5.注入安全提示\n"
                L"为了防止系统崩溃黑屏，NoMoreCapture已自动屏蔽对 explorer.exe、dwm.exe 等系统核心进程的注入。";
            MessageBoxW(hwnd, help_text, L"NoMoreCapture Help", MB_OK | MB_ICONINFORMATION);
            break;
        }
        case ID_EXIT: {
            SendMessage(hwnd, WM_CLOSE, 0, 0);
            break;
        }
        case ID_HIDE: {
            ShowWindow(hwnd, SW_HIDE);
            break;
        }
        case ID_SHOW_MAIN_WINDOW: {
            ShowWindow(hwnd, SW_SHOW);
            break;
        }
        case ID_TRAY_EXIT: {
            SendMessage(hwnd, WM_CLOSE, 0, 0);
            return 0;
        }
        case ID_CALL_PANEL: {
            ShowWindow(ptr_ctx->panel_ctx.panel_hwnd, SW_SHOW);
            break;
        }
        default:
            return DefWindowProc(hwnd, msg, wparam, lparam);
        }
        break;
    }
    case WM_DESTROY: {
        PostQuitMessage(0);
        return 0;
    }
    case WM_HOTKEY: {
        if (wparam == HOTKEY_ID) {
            struct context* ptr_ctx = (struct context*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
            if (ptr_ctx != NULL) {
                // 修正 1：检查 f_e (老板键是否启用)，而不是 2 (白名单模式)
                if (CHECK_BIT(ptr_ctx->panel_ctx.bitwise, f_e) != 0) {

                    // 修正 2：使用独立变量维护状态，每次取反
                    ptr_ctx->panel_ctx.is_boss_key_hidden = !ptr_ctx->panel_ctx.is_boss_key_hidden;
                    int show_cmd = ptr_ctx->panel_ctx.is_boss_key_hidden ? SW_HIDE : SW_SHOW;

                    // 修正 3：隐藏目标窗口（仅针对白名单模式生效，因为全局模式不知道目标是谁）
                    if (CHECK_BIT(ptr_ctx->panel_ctx.bitwise, f_c) != 0) {
                        for (int i = 0; i < 256; i++) {
                            if (ptr_ctx->target[i] != NULL && IsWindow(ptr_ctx->target[i])) {
                                ShowWindow(ptr_ctx->target[i], show_cmd);
                            }
                        }
                    }

                    // 隐藏 Lilith 自己的 UI
                    ShowWindow(ptr_ctx->window_hwnd, show_cmd);
                    if (ptr_ctx->panel_ctx.panel_hwnd) {
                        ShowWindow(ptr_ctx->panel_ctx.panel_hwnd, show_cmd);
                    }
                }
            }
        }
        return 0;
    }
    default:
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
    return 0;
}