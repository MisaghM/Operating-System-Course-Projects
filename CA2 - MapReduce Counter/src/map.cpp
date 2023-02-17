#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "consts.hpp"
#include "csv.hpp"
#include "filesys.hpp"
#include "logger.hpp"
#include "serializer.hpp"

using namespace std::string_literals;
Logger lg("map");

using GenreToBookMap = std::unordered_map<std::string, std::vector<std::string>>;

GenreToBookMap map(const Csv::Table& books, const std::vector<std::string>& genres) {
    GenreToBookMap map;
    for (const auto& genre : genres) {
        map[genre];
    }

    for (const auto& book : books) {
        const auto& name = book[0];
        for (unsigned i = 1; i < book.size(); ++i) {
            auto itr = map.find(book[i]);
            if (itr == map.end()) continue; // skip genre not in genres
            itr->second.push_back(name);
        }
    }

    return map;
}

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        std::cerr << "usage: " << consts::exeMap << " <filename>\n";
        return EXIT_FAILURE;
    }

    auto genres = srz::deserialize(std::cin);

    Csv csv(argv[1]);
    if (csv.readfile()) {
        lg.error("File open error.");
        return EXIT_FAILURE;
    }
    auto books = csv.get();

    GenreToBookMap genreMap = map(books, genres);
    auto pipePath = fs::path(consts::pipesPath);

    for (const auto& book : genreMap) {
        const std::string& genre = book.first;
        int count = book.second.size();

        std::ofstream fifo(pipePath / genre);
        if (!fifo.is_open()) {
            lg.error("Named pipe not found for genre: "s + genre);
            continue;
        }
        fifo << count << '\n';
    }

    return EXIT_SUCCESS;
}
