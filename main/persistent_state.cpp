#include "persistent_state.h"
#include "sd_raw_access.h"
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include "esp_log.h"

namespace PersistentState {
    static const char* TAG = "persistent_state";
    static const char* info_path = "DEI/player_info.txt";

    std::string trim_string(const std::string& str) {
        const std::string whitespace = " \t\n\r\f\v";
        size_t start = str.find_first_not_of(whitespace);
        if (start == std::string::npos) return "";
        size_t end = str.find_last_not_of(whitespace);
        return str.substr(start, end - start + 1);
    }

    bool player_info_exists_on_sd() {
        FILE* fp = sd_raw_fopen(info_path, "r");
        if (fp) {
            sd_raw_fclose(fp);
            return true;
        }
        return false;
    }

    // Helper to join and split read_pages for file storage
    static std::string join_pages(const std::vector<std::string>& pages) {
        std::string out;
        for (size_t i = 0; i < pages.size(); ++i) {
            out += pages[i];
            if (i + 1 < pages.size()) out += ",";
        }
        return out;
    }
    static std::vector<std::string> split_pages(const std::string& s) {
        std::vector<std::string> out;
        size_t start = 0, end;
        while ((end = s.find(',', start)) != std::string::npos) {
            out.push_back(trim_string(s.substr(start, end - start)));
            start = end + 1;
        }
        if (start < s.size()) out.push_back(trim_string(s.substr(start)));
        return out;
    }

    bool read_player_info_from_sd(PlayerInfo &info) {
        FILE* fp = sd_raw_fopen(info_path, "r");
        if (!fp) return false;
        char line[128];
        std::map<std::string, std::string> kv;
        while (fgets(line, sizeof(line), fp)) {
            std::string str_line(line);
            size_t eq = str_line.find('=');
            if (eq != std::string::npos) {
                std::string key = trim_string(str_line.substr(0, eq));
                std::string val = trim_string(str_line.substr(eq+1));
                kv[key] = val;
            }
        }
        sd_raw_fclose(fp);
        info.device_id = kv.count("device_id") ? kv["device_id"] : "0";
        info.read_pages = kv.count("read_pages") ? split_pages(kv["read_pages"]) : std::vector<std::string>{};
        return true;
    }

    bool write_default_player_info_to_sd() {
        FILE* fp = sd_raw_fopen(info_path, "w");
        if (!fp) return false;
        fprintf(fp, "device_id=0\nread_pages=\n");
        sd_raw_fclose(fp);
        return true;
    }
    // Future: add menu read-tracking, etc.
}
