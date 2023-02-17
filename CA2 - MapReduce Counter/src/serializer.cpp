#include "serializer.hpp"

#include <istream>
#include <sstream>

namespace srz {

std::string serialize(const std::vector<std::string>& svec) {
    std::ostringstream sstr;
    for (const auto& s : svec) {
        sstr << s << '\n';
    }
    return sstr.str();
}

std::vector<std::string> deserialize(std::istream& is) {
    std::string line;
    std::vector<std::string> res;
    while (std::getline(is, line)) {
        res.push_back(std::move(line));
    }
    return res;
}

} // namespace srz
