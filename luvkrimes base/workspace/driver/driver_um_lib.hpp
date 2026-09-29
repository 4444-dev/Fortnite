#pragma once
#include "driver_includes.hpp"
#include "driver_shared.hpp"

constexpr uint64_t stack_id = 0xdeed;

constexpr uint64_t nmi_occured = 0x01010101;

constexpr uint32_t caller_signature = 0x6969;

constexpr uint32_t win11_23h2_build = 22631;

typedef __int64(__fastcall* NtUserGetCPD_type)(uint64_t hwnd, uint32_t flags, ULONG_PTR dw_data);
extern "C" NtUserGetCPD_type NtUserGetCPD;

typedef __int64(__fastcall* Win11HookFunc_type)(uint64_t a1, uint64_t a2);
extern "C" Win11HookFunc_type Win11HookFunc;

extern "C" void asm_nmi_wrapper(void);
extern "C" void asm_nmi_restoring(void);
extern "C" __int64 __fastcall asm_call_driver(uint64_t hwnd, uint32_t flags, ULONG_PTR dw_data);
extern "C" __int64 __fastcall asm_call_driver_win11(uint64_t a1, uint64_t a2, uint64_t nmi_restoring_addr);

namespace luvkrimes {
    inline bool inited = false;

    inline bool use_win11_hook = false;

    __int64 send_request(void* cmd);

    bool copy_virtual_memory(uint64_t src_cr3, uint64_t dst_cr3, void* src, void* dst, uint64_t size);

    uint64_t get_cr3(uint64_t pid);

    uint64_t get_module_base(const char* module_name, uint64_t pid);

    uint64_t get_module_size(const char* module_name, uint64_t pid);

    uint64_t get_pid_by_name(const char* name);

    uint64_t get_ldr_data_table_entry_count(uint64_t pid);

    bool get_data_table_entry_info(uint64_t pid, module_info_t* info_array);

    bool hide_driver(void);

    bool unload_driver(void);

    bool ping_driver(void);

    void flush_logs(void);

    inline bool init_luvkrimes_lib() {
        if (inited)
            return true;

        if (!LoadLibraryW(L"user32.dll")) {
            LOG_INFO("Failed to load user32.dll");
            return false;
        }

        HMODULE win32u = LoadLibraryW(L"win32u.dll");
        if (!win32u) {
            LOG_INFO("Failed to get win32u.dll handle");
            return false;
        }

        OSVERSIONINFOW os_info = { 0 };
        os_info.dwOSVersionInfoSize = sizeof(os_info);
        typedef LONG(WINAPI* RtlGetVersion_t)(PRTL_OSVERSIONINFOW);
        RtlGetVersion_t pRtlGetVersion = (RtlGetVersion_t)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion");
        if (pRtlGetVersion) {
            pRtlGetVersion((PRTL_OSVERSIONINFOW)&os_info);
        }

        use_win11_hook = (os_info.dwBuildNumber >= win11_23h2_build);

        if (use_win11_hook) {
            uint64_t handler_address = (uint64_t)GetProcAddress(win32u, "NtUserGetCPD");
            if (!handler_address) {
                LOG_INFO("Failed to resolve NtUserGetCPD for Win11 path");
                return false;
            }
            Win11HookFunc = (Win11HookFunc_type)handler_address;
            NtUserGetCPD = nullptr;
            LOG_INFO("Win11 23H2+ detected (build %d), using 2-arg hook path", os_info.dwBuildNumber);
        }
        else {
            uint64_t handler_address = (uint64_t)GetProcAddress(win32u, "NtUserGetCPD");
            NtUserGetCPD = (NtUserGetCPD_type)handler_address;
            Win11HookFunc = nullptr;
        }

        inited = true;

        if (!ping_driver()) {
            LOG_INFO("Driver is not loaded");
            return false;
        }

        if (!hide_driver()) {
            LOG_INFO("Failed to hide driver");
            return false;
        }

        return true;
    }

    inline bool is_lib_inited(void) {
        return inited;
    }

    inline bool is_hook_ready(void) {
        if (use_win11_hook)
            return Win11HookFunc != nullptr;
        return NtUserGetCPD != nullptr;
    }
};

#pragma pack(push, 1)
struct trap_frame_t {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t rsp;
};
#pragma pack(pop)
