#include "lilith_is_all_you_need.h"

void load_ctx_basic(struct context* ptr_ctx) {
    wchar_t fmt[] = L"%s";
    wchar_t str_class_name[] = L"[Lilith]NoMoreCapture_Class";
    wchar_t str_window_name[] = L"[Lilith]NoMoreCapture";
    StringCchPrintfW(ptr_ctx->class_name, size_str, fmt, str_class_name);
    StringCchPrintfW(ptr_ctx->window_name, size_str, fmt, str_window_name);
    ptr_ctx->proc_func = lilith_wndproc;
    ptr_ctx->window_x = 100;
    ptr_ctx->window_y = 100;
    ptr_ctx->window_height = ptr_ctx->img_y;
    ptr_ctx->window_width = ptr_ctx->img_x;
}

void class_init(WNDCLASSEXW* ptr_class, struct context* ptr_ctx) {
    ptr_class->hIcon = NULL; //TODO：加个好的图标
    ptr_class->hIconSm = NULL; //TODO：一定要一起处理防止缩放问题！
    ptr_class->lpszClassName = ptr_ctx->class_name;
    ptr_class->hInstance = ptr_ctx->instance;
    ptr_class->hCursor = LoadCursor(NULL, IDC_ARROW);
    ptr_class->cbSize = sizeof(WNDCLASSEXW);
    ptr_class->style = CS_HREDRAW | CS_CLASSDC | CS_DBLCLKS;
    ptr_class->lpfnWndProc = lilith_wndproc;
    ptr_class->hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // 存疑
}

bool init_bmi(struct context* ptr_ctx) {
    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFO);
    bmi.bmiHeader.biWidth = ptr_ctx->window_width;
    bmi.bmiHeader.biHeight = -ptr_ctx->window_height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    ptr_ctx->bitmap = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &ptr_ctx->bits, NULL, 0);
    if (ptr_ctx->bitmap == NULL || ptr_ctx->bits == NULL) {
        return false;
    }
    return true;
}

bool init_img_ctx(struct context* ptr_ctx) {
    if (ptr_ctx == NULL) {
        return false;
    }
    HMODULE curr_module = GetModuleHandleW(NULL);
    if (curr_module == NULL) {
        return false;
    }
    HRSRC resource_handle = FindResourceW(curr_module, MAKEINTRESOURCE(IDR_BACKGROUND_PNG), RT_RCDATA);
    if (resource_handle == NULL) {
        out(L"找资源失败", err);
        return false;
    }
    HGLOBAL handle_global = LoadResource(curr_module, resource_handle);
    if (handle_global == NULL) {
        out(L"加载资源失败", err);
        return false;
    }
    DWORD resource_size = SizeofResource(curr_module, resource_handle);
    if (resource_size <= 0) {
        out(L"资源没大小", err);
        return false;
    }
    const void* ptr_data = LockResource(handle_global);
    if (ptr_data == NULL) {
        out(L"资源空的", err);
        return false;
    }
    unsigned char* pixels = stbi_load_from_memory((const stbi_uc*)ptr_data, (int)resource_size, &ptr_ctx->img_x, &ptr_ctx->img_y, &ptr_ctx->cnt_channels, 4);
    ptr_ctx->img_pixels = pixels;
    return true;
}

bool add_tray_icon(struct context* ptr_ctx) {
    if (ptr_ctx == NULL) {
        return false;
    }
    memset(&ptr_ctx->tray_nid, 0, sizeof(NOTIFYICONDATA));
    ptr_ctx->tray_nid.cbSize = sizeof(NOTIFYICONDATA);
    ptr_ctx->tray_nid.hWnd = ptr_ctx->window_hwnd;
    ptr_ctx->tray_nid.uID = 1001;
    ptr_ctx->tray_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    ptr_ctx->tray_nid.uCallbackMessage = WM_TRAYICON;
    ptr_ctx->tray_nid.hIcon = LoadIconW(ptr_ctx->instance, MAKEINTRESOURCEW(IDI_LILITH_ICON));
    StringCchPrintfW(ptr_ctx->tray_nid.szTip, 64, L"NoMoreCapture");
    if (Shell_NotifyIcon(NIM_ADD, &ptr_ctx->tray_nid) == TRUE) {
        return true;
    }
    return false;
}

bool init_panel(struct context *ptr_ctx) {
    ptr_ctx->panel_ctx.panel_hwnd = CreateDialogParamW(ptr_ctx->instance, MAKEINTRESOURCEW(IDD_CONTROL_PANEL), ptr_ctx->window_hwnd, (LPARAM)dlg_proc,ptr_ctx);
    if (ptr_ctx->panel_ctx.panel_hwnd != NULL) {
        //ShowWindow(ptr_ctx->panel_ctx.panel_hwnd, SW_SHOW);
        SetWindowPos(ptr_ctx->panel_ctx.panel_hwnd, HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        return true;
    }
    return false;
}