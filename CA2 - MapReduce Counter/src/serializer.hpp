#ifndef SERIALIZER_HPP_INCLUDE
#define SERIALIZER_HPP_INCLUDE

#include <string>
#include <vector>

namespace srz {

std::string serialize(const std::vector<std::string>& svec);
std::vector<std::string> deserialize(std::istream& is);

} // namespace srz

#endif // SERIALIZER_HPP_INCLUDE
