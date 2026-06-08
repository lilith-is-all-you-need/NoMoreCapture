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

void clean_context(struct context* ptr_ctx) {
    if (ptr_ctx == NULL) {
        return;
    }
    if (ptr_ctx->hdc) DeleteDC(ptr_ctx->hdc);
    if (ptr_ctx->bitmap) DeleteObject(ptr_ctx->bitmap);
    if (ptr_ctx->window_hwnd) DestroyWindow(ptr_ctx->window_hwnd);
    if (ptr_ctx->panel_ctx.pending_ico) DestroyIcon(ptr_ctx->panel_ctx.pending_ico);
    if (ptr_ctx->panel_ctx.using_ico) DestroyIcon(ptr_ctx->panel_ctx.using_ico);
    if (CHECK_BIT(ptr_ctx->panel_ctx.bitwise, f_f) == 1) {
        CLEAR_BIT(ptr_ctx->panel_ctx.bitwise, f_f);
        UnregisterHotKey(ptr_ctx->window_hwnd, HOTKEY_ID);
    }
    Shell_NotifyIconW(NIM_DELETE, &ptr_ctx->tray_nid);
    stbi_image_free(ptr_ctx->img_pixels);
}