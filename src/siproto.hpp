//==========< siproto.hpp >==========
//[Description]: SILang's Prototype
// see Copyright Notice in silang.hpp

#ifndef __SIPROTOTYPE__
#define __SIPROTOTYPE__

#include <string>
#include <vector>
#include <iostream>

#define _SI_ULL unsigned long long
#define type_num "<number>"
#define type_str "<string>"
#define type_bool "<boolean>"
#define type_identifier "<identifier>"
#define type_proc "<procedure>"
#define type_ifelse "<ifelse>"
#define type_array "<array>"

namespace SIProto {
    class Proc {
        _SI_ULL offset = 0;
        _SI_ULL start;
        _SI_ULL end;
        public:
            Proc(_SI_ULL start_idx, _SI_ULL end_idx, _SI_ULL offset) {
                this->start = start_idx;
                this->end = end_idx;
                this->offset = offset;
            }
            inline _SI_ULL get_offset() {return this->offset;}
            inline std::vector<_SI_ULL> get_loc() {return {this->start, this->end};}
    };
    class IfElse {
        _SI_ULL offset = 0;
        _SI_ULL if_start;
        _SI_ULL else_start;
        _SI_ULL end;
        public:
            IfElse(_SI_ULL if_start, _SI_ULL else_start, _SI_ULL end) {
                this->if_start = if_start;
                this->else_start = else_start;
                this->end = end;
            }
            inline void apply_offset(_SI_ULL new_offset){this->offset = new_offset;}
            inline _SI_ULL get_offset() {return this->offset;}
            inline std::vector<_SI_ULL> get_locs() {return {this->if_start, this->else_start, this->end};}
    };
}

namespace SIAbsTree {
    class Node {
        std::string name;
        void *val;
        std::string type;
        public:
            Node *LHS = nullptr;
            Node *RHS = nullptr;
            inline void Del_LHS() {
                if (this->LHS != nullptr) {
                    delete(this->LHS);
                    this->LHS = nullptr;
                }
            };
            inline void Del_RHS() {
                if (this->RHS != nullptr) {
                    delete(this->RHS);
                    this->RHS = nullptr;
                }
            };
            inline std::string get_name() {
                return this->name; 
            };
            inline std::string get_type() { 
                return this->type; 
            };
            inline void *get_val() { 
                return this->val; 
            };
            inline void assign_val(void *new_val, std::string new_type)
            {
                if (this->val)
                {
                    free(this->val);
                    this->val = nullptr;
                }
                this->val = new_val;
                this->type = new_type;
                return;
            };
            Node(const std::string &name) {
                this->name = name;
                this->val = nullptr;
                this->type = -1;
            };
            ~Node(){
                if (this->val) {
                    free(this->val);
                    this->val = nullptr;
                }
                this->Del_LHS();
                this->Del_RHS();
            };
    };

    class Tree {
        Node *root;
        public:
            Tree() {
                this->root = new SIAbsTree::Node("");
            };
            ~Tree() {
                delete (this->root);
                this->root = nullptr;
            };
            Node *getNode(const std::string &name) {
                Node *head = this->root;
                int len = name.length();
                int len2 = head->get_name().length();
                while (1)
                {
                    if (head == nullptr || head->get_name() == name)
                        return head;
                    len2 = head->get_name().length();
                    if (len < len2)
                    {
                        head = head->LHS;
                        continue;
                    }
                    head = head->RHS;
                    continue;
                }
                return head;
            };
            void delNode(Node *to_del) {
                Node *head = this->root;
                int len = to_del->get_name().length();
                int len2 = head->get_name().length();
                while (1)
                {
                    if (head == nullptr)
                        return;
                    if (head == to_del)
                    {
                        if (len < len2)
                        {
                            head->Del_LHS();
                            return;
                        }
                        head->Del_RHS();
                    }
                    len2 = head->get_name().length();
                    if (len < len2)
                    {
                        head = head->LHS;
                        continue;
                    }
                    head = head->RHS;
                    continue;
                }
            };
            void insertNode(Node *to_insert) {
                Node *head = this->root;
                Node *prev = this->root;
                int len = to_insert->get_name().length();
                int len2 = head->get_name().length();
                while (1)
                {
                    if (head == nullptr)
                    {
                        int l = prev->get_name().length();
                        if (len < l)
                        {
                            prev->LHS = to_insert;
                            return;
                        }
                        prev->RHS = to_insert;
                        return;
                    }
                    len2 = head->get_name().length();
                    if (len < len2)
                    {
                        if (head->LHS == nullptr)
                        {
                            head->LHS = to_insert;
                            return;
                        }
                        prev = head;
                        head = head->LHS;
                        continue;
                    }
                    if (head->RHS == nullptr)
                    {
                        head->RHS = to_insert;
                        return;
                    }
                    prev = head;
                    head = head->RHS;
                    continue;
                }
            };
    };
}

namespace SIStack {

    class Val {
        void *val = nullptr;
        std::string type;
        public:
            inline void *get_val() {
                return this->val;
            };
            inline std::string get_type() { 
                return this->type; 
            };
            Val(std::string type, void *val_ptr) {
                this->type = type;
                this->val = val_ptr;
            };
            ~Val() {
                if (this->type != type_proc && this->val)
                    free(this->val);
                this->val = nullptr;
            };
    };
}

#endif