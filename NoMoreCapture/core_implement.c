#include "lilith_is_all_you_need.h"
#include "inject_dep.h"

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

static uint32_t rotl32(uint32_t x, int8_t r) { // 滚动左移
    return (x << r) | (x >> (32 - r));
}
inline uint32_t murmur_hash(const void* key, int len, uint32_t seed) { // 梦开始的地方
    const uint8_t* data = (const uint8_t*)key; // void* 必须强制指定类型才能使用 这里是uint8_t
    const int nblocks = len / 4; // 4个字符分一个块
    uint32_t h1 = seed; // 总hash值

    //试验过的神秘数字
    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    const uint32_t* blocks = (const uint32_t*)(data + nblocks * 4); // 指向最后一个分块的位置
    for (int i = -nblocks; i; i++) {
        uint32_t k1 = blocks[i]; // 当前块

        // 混淆当前块
        k1 *= c1;
        k1 = rotl32(k1, 15);
        k1 *= c2;

        //加入总体hash
        h1 ^= k1;
        h1 = rotl32(h1, 13);
        h1 = h1 * 5 + 0xe6546b64; //线性同余发生器
    }
    const uint8_t* tail = (const uint8_t*)(data + nblocks * 4); // 指向尾部块
    uint32_t k1 = 0;
    switch (len & 3) {  // 处理不满4个的情况 另外len & 3 <=> len % 4
    case 3: k1 ^= tail[2] << 16;
    case 2: k1 ^= tail[1] << 8;
    case 1: k1 ^= tail[0];
        k1 *= c1;
        k1 = rotl32(k1, 15);
        k1 *= c2;
        h1 ^= k1;
    };

    h1 ^= len; // 应对内容为0 但是长度不同的字符串 不发生哈希碰撞
    h1 ^= h1 >> 16; // 循环左移使得高位被处理的次数更多 此时低位可能发生碰撞 因此 分一部分高位优势给地位
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;
    return h1;
}


#pragma pack(push, 1)
struct SHELLCODE_X86 {
    BYTE  push_flag;     // 0x6A (push byte)
    BYTE  flag_val;      // 0x11 (WDA_EXCLUDEFROMCAPTURE)
    BYTE  push_hwnd;     // 0x68 (push dword)
    DWORD hwnd_val;      // 占位符: HWND (4 bytes)
    BYTE  mov_eax;       // 0xB8 (mov eax, dword)
    DWORD api_addr;      // 占位符: API Address (4 bytes)
    WORD  call_eax;      // 0xD0FF (call eax)
    BYTE  ret;           // 0xC3 (ret)
};
#pragma pack(pop)
#pragma pack(push, 1)
struct SHELLCODE_X64 {
    BYTE  sub_rsp[4];    // 48 83 EC 28
    BYTE  mov_rcx[2];    // 48 B9
    ULONG64 hwnd_val;    // 占位符: HWND (8 bytes)
    BYTE  mov_edx[5];    // BA 11 00 00 00
    BYTE  mov_rax[2];    // 48 B8
    ULONG64 api_addr;    // 占位符: API Address (8 bytes)
    WORD  call_rax;      // FF D0
    BYTE  add_rsp[4];    // 48 83 C4 28
    BYTE  ret;           // C3
};
#pragma pack(pop)

struct _IMAGE_NT_HEADERS64* parse_addr_to_nt_headers_x64(void* dll_base) {
    struct _IMAGE_NT_HEADERS64* ptr_nt_headers = NULL;
    if (dll_base == NULL) {
        return NULL;
    }
    struct _IMAGE_DOS_HEADER* ptr_dos_headers = (struct _IMAGE_DOS_HEADER*)dll_base;
    if (ptr_dos_headers == NULL) {
        return NULL;
    }
    ptr_nt_headers = (struct _IMAGE_NT_HEADERS64*)((uintptr_t)dll_base + ptr_dos_headers->e_lfanew);
    return ptr_nt_headers;
}

struct _IMAGE_OPTIONAL_HEADER64* get_ptr_optional_headers_x64(struct _IMAGE_NT_HEADERS64* ptr_nt_headers)
{
    if (ptr_nt_headers == NULL) {
        return NULL;
    }
    struct _IMAGE_OPTIONAL_HEADER64* ptr_opt_headers = &ptr_nt_headers->OptionalHeader;
    return ptr_opt_headers;
}

struct _IMAGE_EXPORT_DIRECTORY* get_ptr_eat_x64(void* dll_base, struct _IMAGE_OPTIONAL_HEADER64* ptr_optional_headers) {
    if (dll_base == NULL || ptr_optional_headers == NULL) {
        return NULL;
    }
    DWORD eat_size = 0;
    DWORD eat_rva = 0;
    eat_rva = ptr_optional_headers->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    eat_size = ptr_optional_headers->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
    if (eat_rva == 0 || eat_size == 0) {
        return NULL;
    }
    struct _IMAGE_EXPORT_DIRECTORY* ptr_eat = (struct _IMAGE_EXPORT_DIRECTORY*)((uintptr_t)dll_base + eat_rva);
    return ptr_eat;
}

struct _IMAGE_NT_HEADERS32* parse_addr_to_nt_headers_x86(void* dll_base) {
    struct _IMAGE_NT_HEADERS32* ptr_nt_headers = NULL;
    if (dll_base == NULL) {
        return NULL;
    }
    struct _IMAGE_DOS_HEADER* ptr_dos_headers = (struct _IMAGE_DOS_HEADER*)dll_base;
    if (ptr_dos_headers == NULL) {
        return NULL;
    }
    ptr_nt_headers = (struct _IMAGE_NT_HEADERS32*)((uintptr_t)dll_base + ptr_dos_headers->e_lfanew);
    return ptr_nt_headers;
}

struct _IMAGE_OPTIONAL_HEADER32* get_ptr_optional_headers_x86(struct _IMAGE_NT_HEADERS32* ptr_nt_headers) {
    if (ptr_nt_headers == NULL) {
        return NULL;
    }
    struct _IMAGE_OPTIONAL_HEADER32* ptr_opt_headers = &ptr_nt_headers->OptionalHeader;
    return ptr_opt_headers;
}

struct _IMAGE_EXPORT_DIRECTORY32* get_ptr_eat_x86(void* dll_base, struct _IMAGE_OPTIONAL_HEADER32* ptr_optional_headers) {
    if (dll_base == NULL || ptr_optional_headers == NULL) {
        return NULL;
    }
    DWORD eat_size = 0;
    DWORD eat_rva = 0;
    eat_rva = ptr_optional_headers->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    eat_size = ptr_optional_headers->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
    if (eat_rva == 0 || eat_size == 0) {
        return NULL;
    }
    struct _IMAGE_EXPORT_DIRECTORY32* ptr_eat = (struct _IMAGE_EXPORT_DIRECTORY32*)((uintptr_t)dll_base + eat_rva);
    return ptr_eat;
}

DWORD get_rva_from_eat_by_hash(void* dll_base, struct _IMAGE_EXPORT_DIRECTORY* ptr_eat, uint32_t func_hash) {
    if (!dll_base || !ptr_eat) return 0;
    DWORD nr_func = ptr_eat->NumberOfFunctions;
    DWORD nr_name = ptr_eat->NumberOfNames;
    DWORD* table_func = (DWORD*)((uintptr_t)dll_base + ptr_eat->AddressOfFunctions);
    DWORD* table_name = (DWORD*)((uintptr_t)dll_base + ptr_eat->AddressOfNames);
    WORD* table_ordinals = (WORD*)((uintptr_t)dll_base + ptr_eat->AddressOfNameOrdinals);

    for (DWORD i = 0; i < nr_name; ++i) {
        const char* ptr_curr_name = (const char*)((uintptr_t)dll_base + table_name[i]);
        int len = strlen(ptr_curr_name);
        uint32_t curr_hash = murmur_hash(ptr_curr_name, len, HASH_SEED);

        if (curr_hash == func_hash) {
            WORD func_idx = table_ordinals[i];
            if (func_idx < nr_func) {
                return table_func[func_idx];
            }
            break;
        }
    }
    return 0; // 没找到
}

/*
struct SHELLCODE_X86 code = {
    0x6A, 0x11,
    0x68, (DWORD)target_hwnd,
    0xB8, (DWORD)api_absolute_address,
    0xD0FF, 0xC3
};
*/

/*
struct SHELLCODE_X64 code = {
    {0x48, 0x83, 0xEC, 0x28},
    {0x48, 0xB9}, (ULONG64)target_hwnd,
    {0xBA, 0x11, 0x00, 0x00, 0x00},
    {0x48, 0xB8}, (ULONG64)api_absolute_address,
    0xD0FF,
    {0x48, 0x83, 0xC4, 0x28},
    0xC3
};
*/

bool init_inject_ctx(struct inject_context* ptr_ctx) {
    if (ptr_ctx == NULL) return false;
    HMODULE x86_user32_handle = LoadLibraryExW(L"C:\\Windows\\SysWOW64\\user32.dll", NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE);
    if (x86_user32_handle == NULL) {
        return false;
    }
    void* x86_user32_base = (void*)((uintptr_t)x86_user32_handle & ~(uintptr_t)3);
    const uint32_t hash_set_window_display_affinity = 1851377508;
    struct _IMAGE_NT_HEADERS32* ptr_nt_header_x86 = parse_addr_to_nt_headers_x86(x86_user32_base);
    struct _IMAGE_OPTIONAL_HEADER32* ptr_opt_header_x86 = get_ptr_optional_headers_x86(ptr_nt_header_x86);
    struct _IMAGE_EXPORT_DIRECTORY* ptr_eat_x86 = get_ptr_eat_x86(x86_user32_base,ptr_opt_header_x86);
    ptr_ctx->x86_rva = get_rva_from_eat_by_hash(x86_user32_base, ptr_eat_x86, hash_set_window_display_affinity);
    HMODULE x64_user32_base = GetModuleHandleW(L"user32.dll");
    struct _IMAGE_NT_HEADERS64* ptr_nt_header_x64 = parse_addr_to_nt_headers_x64(x64_user32_base);
    struct _IMAGE_OPTIONAL_HEADER64* ptr_opt_header_x64 = get_ptr_optional_headers_x64(ptr_nt_header_x64);
    struct _IMAGE_EXPORT_DIRECTORY* ptr_eat_x64 = get_ptr_eat_x64(x64_user32_base, ptr_opt_header_x64);
    ptr_ctx->x64_rva = get_rva_from_eat_by_hash(x64_user32_base, ptr_eat_x64, hash_set_window_display_affinity);
    if (ptr_ctx->x86_rva == 0 || ptr_ctx->x64_rva == 0) {
        return false;
    }
    FreeLibrary(x86_user32_handle);
    return true;
}



ULONG_PTR get_target_proc_dll_base(HANDLE proc_handle,DWORD pid,const wchar_t *target_mod_name) {
    //HANDLE proc_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ |   // 用于枚举模块和检查架构
    //    PROCESS_VM_OPERATION | PROCESS_VM_WRITE |       // 用于内存分配和写入Shellcode
    //    PROCESS_CREATE_THREAD, FALSE, pid);
    if (proc_handle == NULL) {
        return NULL;
    }
    BOOL is_wow_64 = FALSE;
    IsWow64Process(proc_handle, &is_wow_64);
    DWORD filter = is_wow_64 ? LIST_MODULES_32BIT : LIST_MODULES_64BIT;
    HMODULE mods[1024];
    DWORD need;
    wchar_t mod_name[MAX_PATH];
    if (EnumProcessModulesEx(proc_handle, mods, sizeof(mods), &need, filter)) {
        int cnt = need / sizeof(HMODULE);
        for (int i = 0; i < cnt; i++) {
            if (GetModuleBaseNameW(proc_handle, mods[i], mod_name, sizeof(mod_name) / sizeof(wchar_t))){
                if (_wcsicmp(mod_name, target_mod_name) == 0){
                    return (ULONG_PTR)mods[i];
                }
            }

        }
    }
    return 0;
}

/*
注：下面都可以当作放屁 因为都不会生效
原因是win32kfull.sys中的下面的代码
__int64 __fastcall NtUserSetWindowDisplayAffinity(__int64 a1, int a2)
{
  __int64 v4; // rax
  __int64 v5; // rbx
  __int64 v6; // rdi
  __int64 v7; // rcx
  __int64 v8; // rdx

  EnterCrit(0, 1);
  v4 = ValidateHwnd(a1);
  v5 = 0;
  v6 = v4;
  if ( v4 )
  {
    if ( *(_QWORD *)(*(_QWORD *)(v4 + 16) + 424LL) != PsGetCurrentProcessWin32Process() ) // 这一行
    {
      v7 = 5;
LABEL_14:
      UserSetLastError(v7);
      goto LABEL_15;
    }
    v8 = *(_QWORD *)(v6 + 104);
    if ( v8 && (*(_DWORD *)(*(_QWORD *)(v8 + 16) + 1232LL) & 0x40000) != 0 )
      v6 = *(_QWORD *)(v6 + 104);
    if ( !(unsigned int)IsTopLevelWindow(v6) || a2 && (a2 & 0x11) == 0 )
    {
      v7 = 87;
      goto LABEL_14;
    }
    if ( !(unsigned int)SetDisplayAffinity((struct tagWND *)v6) )
    {
      v7 = 8;
      goto LABEL_14;
    }
    v5 = 1;
  }
LABEL_15:
  UserSessionSwitchLeaveCrit();
  return v5;
}
因此除了注入别无他法
*/

static bool build_shellcode_x86(HWND target_hwnd, uintptr_t api_abs_addr, struct SHELLCODE_X86* ptr_out) {
    if (ptr_out == NULL) return false;
    ptr_out->push_flag = 0x6A;
    ptr_out->flag_val = 0x11;          // WDA_EXCLUDEFROMCAPTURE
    ptr_out->push_hwnd = 0x68;
    ptr_out->hwnd_val = (DWORD)(uintptr_t)target_hwnd;
    ptr_out->mov_eax = 0xB8;
    ptr_out->api_addr = (DWORD)api_abs_addr;
    ptr_out->call_eax = 0xD0FF;
    ptr_out->ret = 0xC3;
    return true;
}

static bool build_shellcode_x64(HWND target_hwnd, uintptr_t api_abs_addr, DWORD affinity, struct SHELLCODE_X64* ptr_out) {
    if (ptr_out == NULL) return false;
    // sub rsp, 0x28  ; 预留 shadow space + 对齐
    ptr_out->sub_rsp[0] = 0x48; ptr_out->sub_rsp[1] = 0x83;
    ptr_out->sub_rsp[2] = 0xEC; ptr_out->sub_rsp[3] = 0x28;
    // mov rcx, hwnd
    ptr_out->mov_rcx[0] = 0x48; ptr_out->mov_rcx[1] = 0xB9;
    ptr_out->hwnd_val = (ULONG64)target_hwnd;
    // mov edx, affinity
    ptr_out->mov_edx[0] = 0xBA;
    ptr_out->mov_edx[1] = (BYTE)(affinity & 0xFF);
    ptr_out->mov_edx[2] = (BYTE)((affinity >> 8) & 0xFF);
    ptr_out->mov_edx[3] = (BYTE)((affinity >> 16) & 0xFF);
    ptr_out->mov_edx[4] = (BYTE)((affinity >> 24) & 0xFF);
    // mov rax, api
    ptr_out->mov_rax[0] = 0x48; ptr_out->mov_rax[1] = 0xB8;
    ptr_out->api_addr = (ULONG64)api_abs_addr;
    // call rax
    ptr_out->call_rax = 0xD0FF;
    // add rsp, 0x28
    ptr_out->add_rsp[0] = 0x48; ptr_out->add_rsp[1] = 0x83;
    ptr_out->add_rsp[2] = 0xC4; ptr_out->add_rsp[3] = 0x28;
    // ret
    ptr_out->ret = 0xC3;
    return true;
}

static bool build_shellcode_x86_ex(HWND target_hwnd, uintptr_t api_abs_addr, DWORD affinity, struct SHELLCODE_X86* ptr_out) {
    if (ptr_out == NULL) return false;
    ptr_out->push_flag = 0x6A;
    ptr_out->flag_val = (BYTE)(affinity & 0xFF); // 只支持 0x00 / 0x01 / 0x11 这种小值 满足我们当前所有需求
    ptr_out->push_hwnd = 0x68;
    ptr_out->hwnd_val = (DWORD)(uintptr_t)target_hwnd;
    ptr_out->mov_eax = 0xB8;
    ptr_out->api_addr = (DWORD)api_abs_addr;
    ptr_out->call_eax = 0xD0FF;
    ptr_out->ret = 0xC3;
    return true;
}

static bool is_unsafe_process(DWORD pid) {
    if (pid == 0) return true;
    HANDLE proc_handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (proc_handle == NULL) {
        return true; 
    }

    WCHAR exe_path[MAX_PATH] = { 0 };
    DWORD size = MAX_PATH;
    bool is_unsafe = false;

    if (QueryFullProcessImageNameW(proc_handle, 0, exe_path, &size)) {
        WCHAR* exe_name = wcsrchr(exe_path, L'\\');
        exe_name = exe_name ? exe_name + 1 : exe_path;

        if (_wcsicmp(exe_name, L"explorer.exe") == 0 ||
            _wcsicmp(exe_name, L"dwm.exe") == 0 ||
            _wcsicmp(exe_name, L"cmd.exe") == 0 ||
            _wcsicmp(exe_name, L"conhost.exe") == 0) {
            is_unsafe = true;
        }
    }
    else {
        is_unsafe = true;
    }

    CloseHandle(proc_handle);
    return is_unsafe;
}

bool inject_set_window_display_affinity(HANDLE proc_handle, DWORD pid, HWND target_hwnd, DWORD affinity, struct inject_context* ptr_inject_ctx) {
    if (proc_handle == NULL || target_hwnd == NULL || ptr_inject_ctx == NULL) {
        return false;
    }

    BOOL is_wow_64 = FALSE;
    IsWow64Process(proc_handle, &is_wow_64);

    // 拿到目标进程里 user32.dll 的基址
    ULONG_PTR target_user32_base = get_target_proc_dll_base(proc_handle, pid, L"user32.dll");
    if (target_user32_base == 0) {
        return false;
    }

    // 计算 API 在目标进程中的绝对地址
    uintptr_t api_abs_addr = is_wow_64
        ? (uintptr_t)(target_user32_base + ptr_inject_ctx->x86_rva)
        : (uintptr_t)(target_user32_base + ptr_inject_ctx->x64_rva);

    // 构造 shellcode
    BYTE  buf[sizeof(struct SHELLCODE_X64)] = { 0 }; // 取大的那个 足够装下x86
    SIZE_T code_size = 0;

    if (is_wow_64) {
        struct SHELLCODE_X86* ptr_code = (struct SHELLCODE_X86*)buf;
        if (!build_shellcode_x86_ex(target_hwnd, api_abs_addr, affinity, ptr_code)) {
            return false;
        }
        code_size = sizeof(struct SHELLCODE_X86);
    }
    else {
        struct SHELLCODE_X64* ptr_code = (struct SHELLCODE_X64*)buf;
        if (!build_shellcode_x64(target_hwnd, api_abs_addr, affinity, ptr_code)) {
            return false;
        }
        code_size = sizeof(struct SHELLCODE_X64);
    }

    // 在目标进程申请可执行内存
    LPVOID remote_code = VirtualAllocEx(proc_handle, NULL, code_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (remote_code == NULL) {
        return false;
    }

    // 写入 shellcode
    SIZE_T bytes_written = 0;
    if (!WriteProcessMemory(proc_handle, remote_code, buf, code_size, &bytes_written) || bytes_written != code_size) {
        VirtualFreeEx(proc_handle, remote_code, 0, MEM_RELEASE);
        return false;
    }

    // 创建远程线程 入口点直接是 shellcode 起始
    HANDLE remote_thread = CreateRemoteThread(proc_handle, NULL, 0, (LPTHREAD_START_ROUTINE)remote_code, NULL, 0, NULL);
    if (remote_thread == NULL) {
        VirtualFreeEx(proc_handle, remote_code, 0, MEM_RELEASE);
        return false;
    }

    DWORD wait_ret = WaitForSingleObject(remote_thread, 100);
    bool ok = (wait_ret == WAIT_OBJECT_0);

    CloseHandle(remote_thread);
    VirtualFreeEx(proc_handle, remote_code, 0, MEM_RELEASE);
    return ok;
}

bool do_inject_anti_capture(DWORD pid, HWND target_hwnd, DWORD affinity, struct inject_context* ptr_inject_ctx) {
    if (pid == 0 || target_hwnd == NULL || ptr_inject_ctx == NULL) {
        return false;
    }
    HANDLE proc_handle = OpenProcess(
        PROCESS_QUERY_INFORMATION |
        PROCESS_VM_READ |
        PROCESS_VM_OPERATION |
        PROCESS_VM_WRITE |
        PROCESS_CREATE_THREAD,
        FALSE, pid);
    if (proc_handle == NULL) {
        return false;
    }
    bool ok = inject_set_window_display_affinity(proc_handle, pid, target_hwnd, affinity, ptr_inject_ctx);
    CloseHandle(proc_handle);
    return ok;
}

bool inject_apply_anti_capture(HWND hwnd, struct context* ptr_ctx, struct inject_context* ptr_inject_ctx) {
    if (!IsWindow(hwnd) || ptr_ctx == NULL || ptr_inject_ctx == NULL) {
        return false;
    }
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0) return false;

    DWORD affinity = WDA_EXCLUDEFROMCAPTURE;
    if (CHECK_BIT(ptr_ctx->panel_ctx.bitwise, f_d) == 0) {
        affinity = WDA_MONITOR;
    }
    return do_inject_anti_capture(pid, hwnd, affinity, ptr_inject_ctx);
}

bool inject_remove_anti_capture(HWND hwnd, struct inject_context* ptr_inject_ctx) {
    if (!IsWindow(hwnd) || ptr_inject_ctx == NULL) {
        return false;
    }
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0) return false;
    return do_inject_anti_capture(pid, hwnd, WDA_NONE, ptr_inject_ctx);
}

void apply_anti_capture(HWND hwnd, struct context* ptr_ctx) {
    if (!IsWindow(hwnd)) return;
    if (ptr_ctx == NULL) return;

    // 防御：绝不能动 explorer 的桌面壁纸层和任务栏 否则 DWM 合成层会被打穿
    WCHAR class_name[256] = { 0 };
    GetClassNameW(hwnd, class_name, 256);
    if (wcscmp(class_name, L"Progman") == 0 ||       // 桌面背景
        wcscmp(class_name, L"WorkerW") == 0 ||       // 桌面背景的另一层
        wcscmp(class_name, L"Shell_TrayWnd") == 0)   // 底部任务栏
    {
        return;
    }

    DWORD affinity = WDA_EXCLUDEFROMCAPTURE;
    if (CHECK_BIT(ptr_ctx->panel_ctx.bitwise, f_d) == 0) {
        affinity = WDA_MONITOR;
    }

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0) return;
    if (is_unsafe_process(pid)) {
        return;
    }
    // 自己进程的窗口走老路径 不用绕一圈注入
    if (pid == GetCurrentProcessId()) {
        SetWindowDisplayAffinity(hwnd, affinity);
        return;
    }

    do_inject_anti_capture(pid, hwnd, affinity, &ptr_ctx->inject_ctx);
}


void remove_anti_capture(HWND hwnd, struct context* ptr_ctx) {
    if (!IsWindow(hwnd)) return;

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0) return;
    if (pid == GetCurrentProcessId()) {
        SetWindowDisplayAffinity(hwnd, WDA_NONE);
        return;
    }
    if (is_unsafe_process(pid)) {
        return;
    }
    do_inject_anti_capture(pid, hwnd, WDA_NONE, &ptr_ctx->inject_ctx);
}

bool is_target_window(HWND hwnd, struct context* ptr_ctx) {
    if (!IsWindow(hwnd)) return false;
    if (ptr_ctx == NULL) {
        return false;
    }
    if (hwnd == ptr_ctx->window_hwnd || hwnd == ptr_ctx->panel_ctx.panel_hwnd) {
        return false;
    }
    if (CHECK_BIT(ptr_ctx->panel_ctx.bitwise, f_c) == 0) { // 全局
        return true;
    }
    for (int i = 0; i < 128; i++) {
        if (ptr_ctx->target[i] == hwnd) {
            return true;
        }
    }
    return false;
}

void CALLBACK WinEventProc(
    HWINEVENTHOOK hWinEventHook,
    DWORD event,
    HWND hwnd,
    LONG idObject,
    LONG idChild,
    DWORD dwEventThread,
    DWORD dwmsEventTime){
    if (idObject != OBJID_WINDOW || hwnd == NULL) return;
    if (event == EVENT_OBJECT_SHOW || event == EVENT_OBJECT_CREATE) {
        if (is_target_window(hwnd,g_ptr_ctx)) {
            apply_anti_capture(hwnd,g_ptr_ctx);
        }
    }
}

BOOL CALLBACK enum_windows_proc_add(HWND hwnd, LPARAM lparam){
    struct context* ptr_ctx = (struct context*)lparam;
    if (ptr_ctx == NULL) {
        return TRUE;
    }
    if (!IsWindowVisible(hwnd)) {
        return TRUE;
    }
    if (hwnd == ptr_ctx->window_hwnd || hwnd == ptr_ctx->panel_ctx.panel_hwnd) {
        return TRUE;
    }
    WCHAR class_name[256];
    GetClassNameW(hwnd, class_name, 256);
    if (wcscmp(class_name, L"Progman") == 0 ||    // 桌面背景
        wcscmp(class_name, L"WorkerW") == 0 ||    // 桌面背景的另一层
        wcscmp(class_name, L"Shell_TrayWnd") == 0 // 底部任务栏
        ){
        return TRUE; // 跳过系统底层 UI
    }
    apply_anti_capture(hwnd, ptr_ctx);
    return TRUE;
}

BOOL CALLBACK enum_windows_proc_remove(HWND hwnd, LPARAM lparam) {
    struct context* ptr_ctx = (struct context*)lparam;
    if (ptr_ctx == NULL) {
        return TRUE;
    }
    if (!IsWindowVisible(hwnd)) {
        return TRUE;
    }
    if (hwnd == ptr_ctx->window_hwnd || hwnd == ptr_ctx->panel_ctx.panel_hwnd) {
        return TRUE;
    }
    WCHAR class_name[256];
    GetClassNameW(hwnd, class_name, 256);
    if (wcscmp(class_name, L"Progman") == 0 ||
        wcscmp(class_name, L"WorkerW") == 0 ||
        wcscmp(class_name, L"Shell_TrayWnd") == 0
        ) {
        return TRUE;
    }

    remove_anti_capture(hwnd, ptr_ctx);
    return TRUE;
}