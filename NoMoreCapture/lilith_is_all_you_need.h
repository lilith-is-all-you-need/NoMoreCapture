#pragma once


#include <Windows.h>
#include <stdbool.h>
#include <strsafe.h>
#include <windowsx.h>
#include <stdint.h>
#include <intsafe.h>
#include <intrin.h>
#include "stb_image.h"
#include <psapi.h>
#include "resource.h"
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "psapi.lib")

#define WM_TRAYICON (WM_USER + 1)

enum {
    threshold = 3
};

enum {
    size_str = 128
};

enum {
    HOTKEY_ID = 1145
};

#include "lilith_is_all_you_need.h"

struct panel_context {
    HWND panel_hwnd;
    HWND target_window_hwnd;
    WNDPROC prev_picture_box_proc;
    HICON pending_ico;
    HICON using_ico;
    HCURSOR normal_cur;
    HCURSOR using_cur;

    struct hotkey{
        WORD hotkey;
        BYTE virtual_code;
        BYTE modifiers;
        UINT mod_reg;
    };
    bool is_boss_key_hidden;
    int bitwise;
};

struct inject_context {
    DWORD x86_rva;
    uintptr_t x64_rva;
};


struct context {
    HINSTANCE instance;
    struct basic_info { // 嵌套是非标准写法 但是就这样吧
        wchar_t class_name[size_str];
        wchar_t window_name[size_str];
        WNDPROC proc_func;
        int window_x;
        int window_y;
        int window_width;
        int window_height;
    };
    struct window_ctx {
        HWND window_hwnd;
    };
    struct paint_ctx {
        HBITMAP bitmap;
        HDC hdc;
        void* bits;
    };
    struct img_ctx {
        unsigned char* img_pixels;
        int img_x;
        int img_y;
        int cnt_channels;
    };
    struct misc_ctx {
        POINT pt_mouse_down;
        bool is_mouse_down;
    };
    struct tray_ctx {
        NOTIFYICONDATA tray_nid;
    };
    struct panel_context panel_ctx;
    struct inject_context inject_ctx;
    HWINEVENTHOOK win_hook;
    HWND target[256];
};



LRESULT CALLBACK lilith_wndproc(
    HWND hwnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam
);

INT_PTR dlg_proc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

enum {
    err = 0,
    info = 1,
    warn = 2
};

#define LILITH_DEBUG
#ifdef LILITH_DEBUG
void out(const wchar_t* str, int opt);
#else
#define out(str, opt) ((void)0)
#endif // LILITH_DEBUG

void load_ctx_basic(struct context* ptr_ctx);
void class_init(WNDCLASSEXW* ptr_class, struct context* ptr_ctx);
bool init_bmi(struct context* ptr_ctx);
bool init_img_ctx(struct context* ptr_ctx);
void clean_context(struct context* ptr_ctx);
bool add_tray_icon(struct context* ptr_ctx);
bool init_panel(struct context* ptr_ctx);

//DLGPROC dlg_proc; // 回调函数

#define CHECK_BIT(var, bit_pos)  (((var) >> (bit_pos)) & 1U)
#define SET_BIT(var, bit_pos)    ((var) |= (1U << (bit_pos)))
#define CLEAR_BIT(var, bit_pos)  ((var) &= ~(1U << (bit_pos)))
#define TOGGLE_BIT(var, bit_pos) ((var) ^= (1U << (bit_pos)))

struct context* g_ptr_ctx;

void remove_anti_capture(hwnd, ptr_ctx);
void apply_anti_capture(HWND hwnd, struct context* ptr_ctx);
bool is_target_window(HWND hwnd, struct context* ptr_ctx);
BOOL CALLBACK enum_windows_proc_add(HWND hwnd, LPARAM lparam);
BOOL CALLBACK enum_windows_proc_remove(HWND hwnd, LPARAM lparam);

bool init_inject_ctx(struct inject_context* ptr_ctx);

#define WM_INJECT_FINISHED (WM_USER + 100)

struct inject_task {
    struct context* main_ctx;          // 主上下文
    HWND hwnd_to_remove[256];          // 需要移除注入的窗口
    int remove_count;
    HWND hwnd_to_add[256];             // 需要添加注入的窗口
    int add_count;
    bool is_global_mode;               // 是否是全局模式
    bool remove_global;
};