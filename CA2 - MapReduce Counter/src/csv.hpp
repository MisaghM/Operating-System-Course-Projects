#ifndef CSV_HPP_INCLUDE
#define CSV_HPP_INCLUDE

#include <string>
#include <vector>

class Csv {
public:
    using Table = std::vector<std::vector<std::string>>;

    Csv(std::string filename);

    int readfile();
    const Table& get() const;

private:
    Table table_;
    std::string filename_;
};

#endif // CSV_HPP_INCLUDE
