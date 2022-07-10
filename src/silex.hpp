//==========< silex.hpp >==========
//[Description]: SILang's Lexical Analyzer
// see Copyright Notice in silang.hpp

#ifndef __SILEXER__
#define __SILEXER__

#include "siproto.hpp"
#include <regex>
#include <iostream>
#include <set>
#include <string>

static const std::set<char> SI_SEP = {' ', '\t', '\n', '#'};

static const std::set<std::string> SIK_TK = {
    "add", "sub", "div", "mul",
    "jmp", "gt", "lt", "lt", 
    "eq", "neq", "gteq", "lteq",
    "and", "not", "proc", "end",
    "dref", "print", "dup", "pop",
    "swap", "rotate", "strconcat", "strat",
    "mkarr", "arrat", "arrconcat", "arrpush",
    "arrpop", "mod", "ref", "if",
    "else", "println", "sqrt", "or"
};

enum SIT_TK {
    tk_identifier = 0x101,
    tk_eof = 0xddd,
    tk_kword = 0xcd,
    tk_bool = 0xb001,
    tk_str = 0xc1320,
    tk_number = 0x696,
    tk_failure = 0xdead
};

class SILex_Reader {
    private:
        std::string str;
        _SI_ULL str_len;
        _SI_ULL p;
        int type = -1;
        double val_num;
        std::string val_str;
        inline void SILex_Separator();
    public:
        _SI_ULL line_number;
        SILex_Reader(const std::string &str);
        void SILex_Read();
        inline _SI_ULL current_read_loc();
        inline void new_region(_SI_ULL start, _SI_ULL end);
        inline void flush();
        inline int getToken();
        inline std::string getStrVal();
        inline double getNumVal();
};

inline int SILex_Reader::getToken() {return this->type == -1? tk_eof : this->type;}
inline std::string SILex_Reader::getStrVal() {return this->val_str;}
inline double SILex_Reader::getNumVal() {return this->val_num;}
inline void SILex_Reader::flush() {
    this->val_num = 0;
    this->val_str = "";
    this->p = 0;
    this->line_number = 0;
}


SILex_Reader::SILex_Reader(const std::string &str) {
    this->str = str;
    this->str_len = 0;
    this->p = 0;
    this->val_num = 0;
    this->val_str = "";
    this->line_number = 1;
}
inline void SILex_Reader::new_region(_SI_ULL start, _SI_ULL end) {
    if (end > this->str.length() || end < start) {
        std::cout << "Lex-ERROR: Invalid end index: " + std::to_string(end) + "\n";
        return;
    }
    this->line_number = 1;
    this->p = start;
    this->str_len = end;
}
inline _SI_ULL SILex_Reader::current_read_loc() {return this->p;}

inline void SILex_Reader::SILex_Separator() {
    int buf = this->str[this->p];
    bool comment = false;
    bool reading_str = false;
    while (comment || reading_str || buf == ' ' || buf == '#'|| buf == '\t' || buf == '\n') {
            if (buf == '#')
                comment = true;
            if (buf == '\n') {
                this->line_number++;
                comment = false;
            }
            this->p++;
            if (this->p >= this->str_len)
                return;
            buf = this->str[this->p];
        };
}

void SILex_Reader::SILex_Read() {
    if (this->p < this->str_len) {
        this->SILex_Separator();
        const std::regex num_pattern("[-+]*?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?");
        const std::regex str_pattern("([\"'])(?:(?=(\\\\?))\\2.)*?\\1");
        std::string literal;
        _SI_ULL literal_len = 0;
        bool reading_str = false;
        char str_start = '?';
        while (this->p < this->str_len && (reading_str || SI_SEP.find(this->str[this->p])==SI_SEP.end())) {
            char tmp = this->str[this->p];
            literal += tmp;
            literal_len += 1;
            if (tmp == '"' || tmp == '\'') {
                if (reading_str && str_start == tmp) {
                    this->p++;
                    reading_str = false;
                    break;
                }
                if (!reading_str) {
                    reading_str = true;
                    str_start = tmp;
                }
            }
            this->p++;
        }
        if (reading_str) {
            this->type = tk_failure;
            this->val_str = "Unterminated string literal: " + literal + "...";
            return;
        }
        if (literal_len) {
            if (std::regex_match(literal, str_pattern)) {
                this->val_str = literal.substr(1,literal_len-2);
                this->type = tk_str;
                return;
            }
            this->val_str = literal;
            if (literal == "true" || literal == "false") {
                this->type = tk_bool;
                return;
            }
            if (SIK_TK.find(literal) != SIK_TK.end()) {
                this->type = tk_kword;
                return;
            }
            if (std::regex_match(literal, num_pattern)) {
                this->type = tk_number;
                this->val_num = std::stod(literal.c_str(), 0);
                return;
            }
            this->type = tk_identifier;
        }
    }
    if (this->p >= this->str_len)
        this->type = tk_eof;
}

#endif
