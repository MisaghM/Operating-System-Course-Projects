#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "consts.hpp"
#include "filesys.hpp"
#include "logger.hpp"

using namespace std::string_literals;
Logger lg("reduce");

int getCount(const std::string& genre, int parts) {
    auto pipePath = fs::path(consts::pipesPath);
    std::ifstream fifo(pipePath / genre);
    if (!fifo.is_open()) {
        lg.error("Named pipe not found for genre: "s + genre);
        return EXIT_FAILURE;
    }

    int countAll = 0;
    for (int i = 0; i < parts; ++i) {
        int count = 0;
        fifo >> count;
        countAll += count;
    }

    return countAll;
}

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        std::cerr << "usage: " << consts::exeReduce << " <genre name> <maps count>\n";
        return EXIT_FAILURE;
    }

    std::string genre(argv[1]);
    int parts = std::stoi(argv[2]);

    int countAll = getCount(genre, parts);

    std::cout << genre << ": " << countAll << '\n';
    return EXIT_SUCCESS;
}
