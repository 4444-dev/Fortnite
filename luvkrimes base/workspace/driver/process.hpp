#pragma once
#pragma warning (disable: 4003)
#include "driver_um_lib.hpp"

namespace process {

    inline bool inited = false;

    inline uint64_t owner_pid = 0;

    inline uint64_t owner_cr3 = 0;

    inline std::string target_process_name;

    inline uint64_t target_pid = 0;

    inline uint64_t target_cr3 = 0;

    inline uint64_t target_module_count = 0;

    inline module_info_t* target_modules = 0;

    inline bool init_process(std::string process_name) {

        target_process_name = process_name;

        if (!luvkrimes::init_luvkrimes_lib()) {
            return false;
        }

        if (!luvkrimes::is_lib_inited()) {
            LOG_INFO("Can't init process if the luvkrimes instance is not initialized");
            return false;
        }

        owner_pid = GetCurrentProcessId();
        if (!owner_pid) {
            LOG_INFO("Failed to get pid of owner process");
            return false;
        }

        owner_cr3 = luvkrimes::get_cr3(owner_pid);
        if (!owner_cr3) {
            LOG_INFO("Failed to get cr3 of owner process");
            luvkrimes::flush_logs();
            return false;
        }

        target_pid = luvkrimes::get_pid_by_name(process_name.c_str());
        if (!target_pid) {
            LOG_INFO("Failed to get pid of target process: %s", process_name.c_str());
            luvkrimes::flush_logs();
            return false;
        }

        target_cr3 = luvkrimes::get_cr3(target_pid);
        if (!target_cr3) {
            LOG_INFO("Failed to get initial CR3 for target process");
            return false;
        }

        target_module_count = luvkrimes::get_ldr_data_table_entry_count(target_pid);
        if (!target_module_count) {
            LOG_INFO("Failed get target module count");
            luvkrimes::flush_logs();
            return false;
        }

        target_modules = (module_info_t*)malloc(sizeof(module_info_t) * target_module_count);
        if (!target_modules) {
            LOG_INFO("Failed to alloc memory for modules");
            return false;
        }

        memset(target_modules, 0, sizeof(module_info_t) * target_module_count);

        if (!luvkrimes::get_data_table_entry_info(target_pid, target_modules)) {
            LOG_INFO("Failed getting data table entry info");
            luvkrimes::flush_logs();
            return false;
        }

        inited = true;

        return true;
    }

    inline bool attach_to_proc(std::string process_name) {
        return init_process(process_name);
    }

    template <typename t>
    inline t read(uint64_t src, uint64_t size = sizeof(t)) {
        t buffer{};

        if (!luvkrimes::copy_virtual_memory(target_cr3, owner_cr3, (void*)src, &buffer, size))
            return {};

        return buffer;
    }

    inline bool write(uint64_t dest, void* src, uint64_t size) {

        if (!luvkrimes::copy_virtual_memory(owner_cr3, target_cr3, src, (void*)dest, size))
            return false;

        return true;
    }

    inline bool read_array(void* dest, uint64_t src, uint64_t size) {
        if (!luvkrimes::copy_virtual_memory(target_cr3, owner_cr3, (void*)src, dest, size))
            return false;

        return true;
    }

    inline bool write_array(uint64_t dest, void* src, uint64_t size) {

        if (!luvkrimes::copy_virtual_memory(owner_cr3, target_cr3, src, (void*)dest, size))
            return false;

        return true;
    }

    inline module_info_t get_module(std::string module_name) {
        for (uint64_t i = 0; i < target_module_count - 1; i++) {

            if (strstr(module_name.c_str(), target_modules[i].name)) {
                return target_modules[i];
            }
        }

        return { 0 };
    }

    inline uint64_t get_module_base(std::string module_name) {
        module_info_t module = get_module(module_name);
        return module.base;
    }

    inline uint64_t get_module_size(std::string module_name) {
        module_info_t module = get_module(module_name);
        return module.size;
    }

    inline void log_modules(void) {
        for (uint64_t i = 0; i < target_module_count - 1; i++) {
            LOG_INFO("%s", target_modules[i].name);
        }
    }

    inline uint64_t get_pid(std::string process_name) { return luvkrimes::get_pid_by_name(process_name.c_str()); }

};
