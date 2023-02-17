#ifndef FILESYS_HPP_INCLUDE
#define FILESYS_HPP_INCLUDE

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace fsys {

int listdir(const std::string& path, std::vector<fs::path>& res);
void filter(std::vector<fs::path>& vec, fs::file_type type);

} // namespace fsys

#endif // FILESYS_HPP_INCLUDE
