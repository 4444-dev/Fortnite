#pragma once

#include <includes.hpp>

#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>
#include "intrin.h"
#include <iostream>
#include <winternl.h>
#include <vector>
#include <filesystem>
#include <fstream>
#include <atomic>

uintptr_t StoredBase;
uintptr_t cr3;

extern "C" __int64 direct_device_control(HANDLE, HANDLE, PIO_APC_ROUTINE, PVOID, PIO_STATUS_BLOCK, std::uint32_t, PVOID, std::uint32_t, PVOID, std::uint32_t);

#define ItpBase 0x4000

#define GetBaseAddressMagic 512545672
#define ProcessIdMagic 112871895
#define addressMagic 101579903
#define BufferMagic 24963771
#define Sizeagic 438549362

#define ReadWritePhysical  CTL_CODE(ItpBase, 0x5AD47, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define GetBaseAddress     CTL_CODE(ItpBase, 0x47b03, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define CR3                CTL_CODE(ItpBase, 0x26Eb8, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define ClearTrace         CTL_CODE(ItpBase, 0x86Eb7, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define MouseCallBack      CTL_CODE(ItpBase, 0x70049, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define DriverVerify       0x987B3C

namespace structs {

	typedef struct _Srw {
		INT32 verify;
		INT32 pid;
		ULONGLONG adr;
		ULONGLONG bfr;
		ULONGLONG si;
		BOOLEAN rw;
	} Srw, * PSrw;

	typedef struct _Ba {
		INT32 Verfy;
		INT32 Pid;
		ULONGLONG* adr;
	} Ba, * PBa;

	typedef struct _ga {
		INT32 security;
		ULONGLONG* address;
	} ga, * pga;

	typedef struct _MEMORY_OPERATION_DATA {
		uint32_t pid;
		ULONGLONG* cr3;
	} MEMORY_OPERATION_DATA, * PMEMORY_OPERATION_DATA;

	typedef struct _cr3 {
		INT32 process_id;
	} cr3, * DTBStruct;

	typedef struct _MU {
		long x;
		long y;
		unsigned short button_flags;
		ULONG ExtraInformation;
	} MU, * PMU;

	struct ProcessData {
		ULONG pid;
	};

	typedef struct _handle_information {
		wchar_t name[100];
		unsigned long stamp;
	} handle_information, * phandle_information;

}

namespace Driver {
	HANDLE driver_handle;
	INT32 process_id;

	bool initDevice() {
		driver_handle = CreateFileW((L"\\\\.\\IPT"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

		if (!driver_handle || (driver_handle == INVALID_HANDLE_VALUE))
			return false;

		return true;
	}

	template<typename T>
	inline bool SendControl(uint32_t ioctl_code, T* args) {
		DWORD bytes_returned = 0;
		return DeviceIoControl(driver_handle, ioctl_code, args, sizeof(T), args, sizeof(T), &bytes_returned, nullptr);
	}

	bool clear_trace(const wchar_t* name, unsigned long stamp) {
		structs::handle_information info{ 0 };
		info.stamp = stamp;
		if (wcslen(name) < 100) wcscpy_s(info.name, name);

		DWORD r = 0;
		BOOL ret = DeviceIoControl(driver_handle, ClearTrace, &info, sizeof(info), 0, 0, &r, 0);

		return ret == TRUE;
	}

	void ReadPhysicalMemory(PVOID address, PVOID buffer, DWORD size) {
		structs::_Srw arguments{};
		arguments.verify = DriverVerify;
		arguments.pid = process_id - ProcessIdMagic;
		arguments.adr = (uint64_t)address - addressMagic;
		arguments.bfr = (uint64_t)buffer - BufferMagic;
		arguments.si = size - Sizeagic;
		arguments.rw = FALSE;

		IO_STATUS_BLOCK iosb{};
		SendControl(ReadWritePhysical, &arguments);
	}

	void WritePhysicalMemory(PVOID address, PVOID buffer, DWORD size) {
		structs::_Srw arguments{};
		arguments.verify = DriverVerify;
		arguments.pid = process_id - ProcessIdMagic;
		arguments.adr = (uint64_t)address - addressMagic;
		arguments.bfr = (uint64_t)buffer - BufferMagic;
		arguments.si = size - Sizeagic;
		arguments.rw = TRUE;

		IO_STATUS_BLOCK iosb{};
		SendControl(ReadWritePhysical, &arguments);
	}

	uintptr_t FindBaseAddress() {
		uintptr_t image_address = 0;

		structs::_Ba arguments{};
		arguments.Verfy = DriverVerify;
		arguments.Pid = process_id - GetBaseAddressMagic;
		arguments.adr = (ULONGLONG*)&image_address;

		IO_STATUS_BLOCK iosb{};
		SendControl(GetBaseAddress, &arguments);

		return image_address;
	}

	intptr_t CachedCr3() {
		uintptr_t out = 0;

		structs::MEMORY_OPERATION_DATA arguments{};
		arguments.pid = process_id;
		arguments.cr3 = (uint64_t*)&out;

		IO_STATUS_BLOCK iosb{};
		SendControl(CR3, &arguments);
		return out;
	}

	INT32 FindProcess(LPCTSTR process_name) {
		PROCESSENTRY32 pt;
		HANDLE hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		pt.dwSize = sizeof(PROCESSENTRY32);
		if (Process32First(hsnap, &pt)) {
			do {
				if (!lstrcmpi(pt.szExeFile, process_name)) {
					CloseHandle(hsnap);
					process_id = pt.th32ProcessID;
					return pt.th32ProcessID;
				}
			} while (Process32NextW(hsnap, &pt));
		}
		CloseHandle(hsnap);

		return { NULL };
	}

	std::atomic<ULONG_PTR> g_currentExtraInfo(0);

	LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
		if (nCode >= 0) {
			MSLLHOOKSTRUCT* pMouseStruct = (MSLLHOOKSTRUCT*)lParam;
			if (!(pMouseStruct->flags & LLMHF_INJECTED)) {
				ULONG_PTR extraInfo = pMouseStruct->dwExtraInfo;
				if (extraInfo != 0) {
					static ULONG_PTR lastValue = 0;
					if (extraInfo != lastValue) {
						lastValue = extraInfo;
						g_currentExtraInfo.store(extraInfo, std::memory_order_release);
					}
				}
			}
		}
		return (CallNextHookEx)(NULL, nCode, wParam, lParam);
	}

	void HookThread() {
		HHOOK hook = (SetWindowsHookExW)(WH_MOUSE_LL, MouseHookProc, NULL, 0);
		if (hook == NULL) return;

		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		UnhookWindowsHookEx(hook);
	}

	void StartHookThread() {
		HANDLE hThread = (CreateThread)(NULL, 0, [](LPVOID) -> DWORD { HookThread(); return 0; }, NULL, 0, NULL);
		CloseHandle(hThread);
	}

	void mouse_event(long x, long y, unsigned short button_flags) {
		ULONG_PTR extraInfo = g_currentExtraInfo.load(std::memory_order_acquire);
		ULONG_PTR prevExtra = SetMessageExtraInfo(extraInfo);

		structs::MU mouse_request{};
		mouse_request.x = x;
		mouse_request.y = y;
		mouse_request.button_flags = button_flags;
		mouse_request.ExtraInformation = extraInfo;

		IO_STATUS_BLOCK iosb{};
		SendControl(MouseCallBack, &mouse_request);
		SetMessageExtraInfo(prevExtra);
	}

}

class GalaxyMem {
public:

	template <typename T>
	[[nodiscard]] T read(uint64_t address) {
		T buffer{};
		Driver::ReadPhysicalMemory((PVOID)address, &buffer, sizeof(T));
		return buffer;
	}

	template <typename T>
	[[nodiscard]] T write(uint64_t address, T buffer) {
		Driver::WritePhysicalMemory((PVOID)address, &buffer, sizeof(T));
		return buffer;
	}

	bool is_valid(const uint64_t address) {
		if (address == 0 || address == 0xCCCCCCCCCCCCCCCC || address == 0xFFFFFFFFFFFFFFFF)
			return false;

		if (address <= 0x400000 || address > 0x7FFFFFFFFFFFFFFF)
			return false;

		return true;
	}

	template<typename T>
	bool read_batch(uintptr_t base, const uint32_t* offsets, T* outputs, size_t count) {
		for (size_t i = 0; i < count; ++i) {
			outputs[i] = read<T>(base + offsets[i]);
			if (!is_valid(outputs[i])) return false;
		}
		return true;
	}

	template<typename T>
	auto ReadArray(uintptr_t address, T out[], size_t len) -> bool {
		for (size_t i = 0; i < len; ++i) {
			out[i] = read<T>(address + i * sizeof(T));
		}
		return true;
	}

	inline bool read1(const std::uintptr_t address, void* buffer, const std::size_t size) {
		if (buffer == nullptr || size == 0) {
			return false;
		}
		Driver::ReadPhysicalMemory(reinterpret_cast<PVOID>(address), buffer, static_cast<DWORD>(size));
		return true;
	}

	template<typename T>
	auto ReadArray2(uint64_t address, T* out, size_t len) -> bool {
		if (!out || len == 0) {
			return false;
		}

		for (size_t i = 0; i < len; ++i) {
			if (!is_valid(address + i * sizeof(T))) {
				return false;
			}

			out[i] = read<T>(address + i * sizeof(T));
		}
		return true;
	}

}; static GalaxyMem* Glxymem = new GalaxyMem();

bool AttachDriver(const wchar_t* ProcessName, uptr* ImageBase) {
	if (!ProcessName || !ImageBase) {
		return false;
	}

	*ImageBase = 0;

	if (!Driver::initDevice()) {
		return false;
	}

	if (!Driver::FindProcess(ProcessName)) {
		return false;
	}

	StoredBase = Driver::FindBaseAddress();
	if (!StoredBase) {
		return false;
	}

	cr3 = static_cast<uintptr_t>(Driver::CachedCr3());

	Unreal::ReadMemory = [](void* Dst, uptr Src, u64 Size) -> bool {
		if (!Dst || !Size) {
			return false;
		}

		Driver::ReadPhysicalMemory(reinterpret_cast<PVOID>(Src), Dst, static_cast<DWORD>(Size));
		return true;
	};

	*ImageBase = static_cast<uptr>(StoredBase);
	return true;
}
