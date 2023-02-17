#include "filesys.hpp"

#include <algorithm>

namespace fsys {

int listdir(const std::string& path, std::vector<fs::path>& res) {
    const fs::path pathfs = path;
    const fs::file_status stats = fs::status(pathfs);

    if (stats.type() == fs::file_type::not_found) return -1;
    if (stats.type() != fs::file_type::directory) return -2;

    res.clear();
    for (const fs::directory_entry& entry : fs::directory_iterator(pathfs)) {
        res.push_back(entry.path());
    }

    return res.size();
}

void filter(std::vector<fs::path>& vec, fs::file_type type) {
    vec.erase(std::remove_if(vec.begin(), vec.end(), [type](const fs::path& p) {
        return fs::status(p).type() != type;
    }), vec.end());
}

} // namespace fsys
