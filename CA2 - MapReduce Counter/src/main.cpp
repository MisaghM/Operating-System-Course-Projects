#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <regex>
#include <vector>

#include "consts.hpp"
#include "csv.hpp"
#include "filesys.hpp"
#include "logger.hpp"
#include "serializer.hpp"

using namespace std::string_literals;
Logger lg("main");

int getDirFiles(const char* path, std::vector<fs::path>& out) {
    int res = fsys::listdir(path, out);
    if (res == -1) {
        lg.error("Library path does not exist.");
        return 1;
    }
    if (res == -2) {
        lg.error("Library path is not a directory.");
        return 1;
    }
    fsys::filter(out, fs::file_type::regular);
    return 0;
}

int getRequiredFiles(std::vector<fs::path>& files, fs::path& genres) {
    auto genresFile = std::find_if(files.begin(), files.end(), [](const fs::path& p) {
        return p.filename() == consts::fileGenres;
    });
    if (genresFile == files.end()) {
        lg.error("Genres file not found.");
        return 1;
    }
    genres = *genresFile;

    // clang-format off
    std::regex rgx(consts::filePattern);
    files.erase(std::remove_if(files.begin(), files.end(), [&rgx](const fs::path& p) {
        return !std::regex_match(p.filename().string(), rgx);
    }), files.end());
    // clang-format on

    if (files.empty()) {
        lg.error("No book part file found.");
        return 1;
    }
    return 0;
}

int readGenres(const fs::path& genresPath, std::vector<std::string>& genres) {
    Csv csv(genresPath.string());
    csv.readfile();
    auto tbl = csv.get();

    if (tbl.size() < 1) {
        lg.error("Genres file empty.");
        return 1;
    }
    genres = tbl[0];
    return 0;
}

int reducers(int filesCount, const std::vector<std::string>& genres) {
    auto pipePath = fs::path(consts::pipesPath);
    for (const auto& genre : genres) {
        unlink((pipePath / genre).c_str());
        if (mkfifo((pipePath / genre).c_str(), 0777) < 0) {
            lg.perrno();
            return 1;
        }

        int forkpid = fork();
        if (forkpid == 0) {
            execl(consts::exeReduce, consts::exeReduce,
                  genre.c_str(),
                  std::to_string(filesCount).c_str(),
                  nullptr);
            lg.error("Failed to exec: "s + consts::exeReduce);
            exit(EXIT_FAILURE);
        }
        else if (forkpid < 0) {
            lg.error("Failed to fork. (reduce)");
            return 1;
        }
    }
    return 0;
}

int mappers(const std::vector<fs::path>& files,
            const std::vector<std::string>& genres) {
    std::string genresSrz = srz::serialize(genres);
    for (const auto& file : files) {
        int pipefd[2];
        if (pipe(pipefd) < 0) {
            lg.error("Failed to pipe. (map)");
            return 1;
        }

        int forkpid = fork();
        if (forkpid > 0) {
            close(pipefd[0]);
            write(pipefd[1], genresSrz.c_str(), genresSrz.size());
            close(pipefd[1]);
        }
        else if (forkpid == 0) {
            close(pipefd[1]);
            close(STDIN_FILENO);
            dup(pipefd[0]);
            close(pipefd[0]);
            execl(consts::exeMap, consts::exeMap, file.c_str(), nullptr);
            lg.error("Failed to exec: "s + consts::exeMap);
            exit(EXIT_FAILURE);
        }
        else {
            lg.error("Failed to fork. (map)");
            return 1;
        }
    }
    return 0;
}

int workers(const std::vector<fs::path>& files,
            const std::vector<std::string>& genres) {
    auto pipePath = fs::path(consts::pipesPath);
    fs::remove_all(pipePath);
    fs::create_directories(pipePath);

    if (reducers(files.size(), genres)) return 1;
    sleep(2);
    if (mappers(files, genres)) return 1;

    int status;
    while (wait(&status) > 0) {}

    fs::remove_all(pipePath);
    return 0;
}

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        std::cerr << "usage: " << consts::programName << " <library folder>\n";
        return EXIT_FAILURE;
    }

    std::vector<fs::path> files;
    if (getDirFiles(argv[1], files)) return EXIT_FAILURE;

    fs::path genresPath;
    if (getRequiredFiles(files, genresPath)) return EXIT_FAILURE;

    std::vector<std::string> genres;
    if (readGenres(genresPath, genres)) return EXIT_FAILURE;

    if (workers(files, genres)) return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
