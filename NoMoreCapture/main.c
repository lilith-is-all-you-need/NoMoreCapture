#include "lilith_is_all_you_need.h"

inline void dbg_ulw(struct context* ptr_ctx) {
    if (ptr_ctx == NULL || ptr_ctx->bits == NULL) {
        return;
    }
    BYTE* ptr_bits = ptr_ctx->bits;
    for (int i = 0; i < ptr_ctx->window_height * ptr_ctx->window_width; i++) {
        //内存布局 B:xxxx(4words)G:xxxx(4words)R:xxxx(4words)A:xxxxx(4words)B:xxxx(4words)G:xxxx(4words)R:xxxx(4words)A:xxxxx(4words)...
        ptr_bits[i * 4 + 0] = 0; // B
        ptr_bits[i * 4 + 1] = 0; // G
        ptr_bits[i * 4 + 2] = 255; // R
        ptr_bits[i * 4 + 3] = 128; // A
        // i 实际上是控制第几组(B,G,R,A)
    }
}

inline void draw_png_with_premulitply_alpha(struct context* ptr_ctx) {
    if (ptr_ctx == NULL || ptr_ctx->img_pixels == NULL) {
        return;
    }
    BYTE* ptr_img = ptr_ctx->img_pixels;
    BYTE* ptr_bits = ptr_ctx->bits;
    for (int i = 0; i < ptr_ctx->img_x * ptr_ctx->img_y; i++) {
        BYTE r = ptr_img[i * 4 + 0]; // r
        BYTE g = ptr_img[i * 4 + 1]; // g
        BYTE b = ptr_img[i * 4 + 2]; // b
        BYTE a = ptr_img[i * 4 + 3]; // a
        ptr_bits[i * 4 + 0] = b * a/ 255; // B
        ptr_bits[i * 4 + 1] = g * a / 255; // G
        ptr_bits[i * 4 + 2] = r * a / 255; // R
        ptr_bits[i * 4 + 3] = a; // A
    }
}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    //INITCOMMONCONTROLSEX icex;
    //icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    //icex.dwICC = ICC_HOTKEY_CLASS | ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES;
    //InitCommonControlsEx(&icex);

    struct context ctx = { 0 };
    ctx.instance = hInstance; // 装入instance
    bool ret = false;
    BOOL win_ret = 0;
    init_img_ctx(&ctx);
    load_ctx_basic(&ctx);
    WNDCLASSEXW wnd_class = { 0 };
    class_init(&wnd_class,&ctx);
    if (RegisterClassExW(&wnd_class) == 0) {
        //失败了
        out(L"注册窗口类失败", err);
        return -1;
    }

    //SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    DWORD ext_style = WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TOPMOST;
    DWORD style = WS_POPUP;
    ctx.window_hwnd = CreateWindowExW(ext_style, ctx.class_name, ctx.window_name, style, ctx.window_x, ctx.window_y, ctx.window_width, ctx.window_height, NULL, NULL, ctx.instance, &ctx);
    if (ctx.window_hwnd == NULL) {
        out(L"创建窗口失败", err);
        return -1;
    }

    if (init_panel(&ctx) == false) {
        out(L"创建Control Panel失败", err);
        return -1;
    }

    if (init_bmi(&ctx) == false) {
        out(L"初始化DIBSection失败", err);
        return -1;
    }

    if (add_tray_icon(&ctx) == false) {
        out(L"初始化托盘图标失败", err);
        return -1;
    }

    // trick:SetWindowDisplay内部实现会检查有没有LAYERED 这样绕过去 且时机可能很重要
    LONG_PTR curr_ex_style = GetWindowLongPtr(ctx.window_hwnd, GWL_EXSTYLE);
    SetWindowLongPtr(ctx.window_hwnd, GWL_EXSTYLE, curr_ex_style & ~WS_EX_LAYERED);
    SetWindowDisplayAffinity(ctx.window_hwnd, WDA_EXCLUDEFROMCAPTURE);
    SetWindowLongPtr(ctx.window_hwnd, GWL_EXSTYLE, curr_ex_style); 

    if (ctx.panel_ctx.panel_hwnd != NULL) {
        SetWindowDisplayAffinity(ctx.panel_ctx.panel_hwnd, WDA_EXCLUDEFROMCAPTURE);
    }
    
    if (init_inject_ctx(&ctx.inject_ctx) == false) {
        out(L"初始化注入器失败", err);
        return -1;
    }

    g_ptr_ctx = &ctx; // 为了给钩子
    ctx.hdc = CreateCompatibleDC(NULL);
    SelectObject(ctx.hdc, ctx.bitmap);
    POINT window_pt = { ctx.window_x,ctx.window_y };
    POINT start_pt = { 0,0 };
    SIZE window_size = { ctx.window_width,ctx.window_height };
    BLENDFUNCTION blend_opt = { 0 };
    draw_png_with_premulitply_alpha(&ctx);
    blend_opt.AlphaFormat = AC_SRC_ALPHA;
    blend_opt.SourceConstantAlpha = 255;
    blend_opt.BlendOp = AC_SRC_OVER;
    if (UpdateLayeredWindow(ctx.window_hwnd, NULL, &window_pt, &window_size,ctx.hdc, &start_pt, 0, &blend_opt, ULW_ALPHA) == 0){
        out(L"初次ULW失败", err);
        return -1;
    }
    ShowWindow(ctx.window_hwnd, SW_SHOW);
    
    //消息循环
    MSG curr_msg;
    while (GetMessage(&curr_msg, NULL, 0, 0) > 0) {
        if (ctx.panel_ctx.panel_hwnd == NULL || !IsDialogMessage(ctx.panel_ctx.panel_hwnd, &curr_msg)) { // 优先让非模态对话框处理
            TranslateMessage(&curr_msg);
            DispatchMessageW(&curr_msg);
        }
    }
    clean_context(&ctx);
    return 0;
}