//==========< silex.hpp >==========
//[Description]: SILang's Interpreter
// see Copyright Notice in silang.hpp

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "silang.hpp"

inline bool valid_path(const std::string& path) {
    std::ifstream temp(path.c_str());
    return temp.good();
}

int main(int argc, char **argv)
{   
    SIVM *sivm = new SIVM();
    if (argc > 1) {
        std::string arg(argv[1]);
        if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: silang [option] [file?]\n";
            std::cout << "Available options are:\n";
            std::cout << "   -h             Display this help information. [--help]\n";
            std::cout << "   -v             Display SILang's version. [--version]\n";
            std::cout << "   -f [file_path] Run file. [--file]\n";
            return 0;
        }
        if (arg == "--version" || arg == "-v") {
            std::cout << SILANG_COPYRIGHT << "\n";
            return 0;
        }
        if (arg == "--file" || arg == "-f") {
            if (argc >= 3) {
                std::string file_path = argv[2];
                if (valid_path(file_path)) {
                    std::ifstream file(file_path);
                    std::stringstream buf;
                    buf << file.rdbuf();
                    file.close();
                    sivm->feed(buf.str());
                    sivm->exec();
                    return 0;
                }
                std::cout << "ERROR: Invalid path to file: '" + file_path + "' (-h for help)\n";
                return 1;
            }
            std::cout << "ERROR: No input file.\n";
            return 1;
        }
        std::cout << "ERROR: Unknown option: " << argv[1] << "\n";
        return 0;
    }
    std::cout << SILANG_COPYRIGHT << "\nType \".exit\" to exit.\n";
    while (1) {
        std::string input_buffer;
        std::cout << ">>> ";
        getline(std::cin, input_buffer);
        if (input_buffer == ".exit")
            break;
        sivm->feed(input_buffer);
        sivm->exec();
    }
    return 0;
}