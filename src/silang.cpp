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
    if (argc > 1) {
        std::string arg(argv[1]);
        if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: silang [option/file]\n";
            std::cout << "Available options are:\n";
            std::cout << "   -h         Display this help information. [--help]\n";
            std::cout << "   -v         Display SILang's version. [--version]\n";
            return 0;
        }
        if (arg == "--version" || arg == "-v") {
            std::cout << SILANG_COPYRIGHT << "\n";
            return 0;
        }
        if (valid_path(arg)) {
            std::ifstream file(arg);
            std::stringstream buf;
            buf << file.rdbuf();
            file.close();
            SIVM v = SIVM();
            v.feed(buf.str());
            v.exec();
            return 0;
        }
        std::cout << "ERROR: Invalid path to file: '" + arg + "' (-h for help)\n";
        return 1;
    }
    std::cout << "ERROR: No input files." << " (-h for help)\n";
    return 1;
}