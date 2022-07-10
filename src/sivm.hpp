//==========< sivm.hpp >==========
//[Description]: SILang's Virtual Machine
// see Copyright Notice in silang.hpp

#ifndef __SIVM__
#define __SIVM__

#include <iostream>
#include <cmath>
#include <memory>
#include <vector>
#include <stack>
#include <set>
#include <string>
#include "silex.hpp"
#include "siproto.hpp"

class SIVM {
    private:
        std::string si_buf;
        bool _feeded_ = false;
        bool _proto_init_ = false;

        std::unique_ptr<SILex_Reader> reader = std::make_unique<SILex_Reader>("");
        std::unique_ptr<SIAbsTree::Tree> Heapy = std::make_unique<SIAbsTree::Tree>();
        std::unique_ptr<std::stack<SIStack::Val*>> Stacky = std::make_unique<std::stack<SIStack::Val*>>();
        _SI_ULL line_offset = 0;

        inline int nextToken()
        {
            if (!this->_feeded_)
            {
                std::cout << "ERROR: VM isn't initialized.\n";
                return -1;
            }
            this->reader->SILex_Read();
            //std::cout << this->reader->getToken() << "  '" << this->reader->getStrVal() << "'   " << this->reader->getNumVal() << "\n";
            return this->reader->getToken();
        };

        // error logger
        inline void ErrorLog(const std::string &error_message)
        {
            std::cout << "ERROR:" << this->line_offset - (this->line_offset - this->reader->line_number) << ": " << error_message;
            if (this->_proto_init_)
                std::cout << " [Near: '" + this->reader->getStrVal() <<"']";
            this->_feeded_ = false;
            this->_proto_init_ = false;
            std::cout << "\n";
        };
        // Common errors
        inline void ErrorLog_STACKEMPTY() {
            this->ErrorLog("Stack is empty.");
        }
        inline void ErrorLog_EXPECTEDVAL(const std::string &expected_val, const std::string &got_val) {
            this->ErrorLog("Expected " + expected_val + " but got " + got_val + " instead.");
        }
        inline void ErrorLog_UNKNOWNREF(const std::string &ref_name) {
            this->ErrorLog("Unknown reference '" + ref_name + "'.");
        }
        inline void ErrorLog_NOMEM() {
            this->ErrorLog("Not enough memory.");
        }

        // Prototype initialization
        void Proto_Initialize() {
            std::vector<std::pair<std::string, _SI_ULL>> startp;
            _SI_ULL total_startp = 0;
            while (this->_feeded_) {
                this->nextToken();
                if (this->reader->getToken() == tk_eof) break;
                if (this->reader->getToken() == tk_failure) {
                    this->ErrorLog(this->reader->getStrVal());
                    return;
                }
                if (this->reader->getToken() == tk_kword) {
                    std::string wkwrd = this->reader->getStrVal();
                    if (wkwrd != "proc" && wkwrd != "else" && wkwrd != "if" && wkwrd != "end")
                        continue;
                    if (wkwrd == "proc") {
                        std::string proc_name;
                        this->nextToken();
                        if (this->reader->getToken() == tk_failure) {
                            this->ErrorLog(this->reader->getStrVal());
                            return;
                        }
                        if (this->reader->getToken() != tk_identifier) {
                            this->ErrorLog("Invalid procedure's name: '" + this->reader->getStrVal() + "'.");
                            return;
                        }
                        proc_name = this->reader->getStrVal();
                        total_startp++;
                        startp.push_back(std::pair<std::string, _SI_ULL>(
                            proc_name,
                            this->reader->current_read_loc()
                        ));
                        continue;
                    }
                    if (wkwrd == "if") {
                        total_startp++;
                        startp.push_back(std::pair<std::string, _SI_ULL>(
                            "",
                            this->reader->current_read_loc()
                        ));
                        continue;
                    }
                    if (wkwrd == "else") {
                        total_startp++;
                        startp.push_back(std::pair<std::string, _SI_ULL>(
                            " ",
                            this->reader->current_read_loc()
                        ));
                        continue;
                    }
                    if (wkwrd == "end") {
                        if (total_startp > 0) {
                            auto sp = startp[total_startp-1];
                            auto pname = sp.first;
                            auto loc = sp.second;
                            auto loc2 = this->reader->current_read_loc()-3;
                            if (pname == "") {
                                auto ifnode = new SIAbsTree::Node(std::to_string(loc));
                                auto proto = new SIProto::IfElse(loc, 0, loc2);
                                proto->apply_offset(this->reader->line_number);
                                ifnode->assign_val(proto, type_ifelse);
                                this->Heapy->insertNode(ifnode);
                            } else if (pname == " ") {
                                startp.pop_back();
                                total_startp--;
                                if (!total_startp || startp[total_startp-1].first != "") {
                                    this->ErrorLog("Expected <if> before <else>.");
                                    return;
                                }
                                auto ifnode = startp[total_startp-1];
                                auto ifelsenode = new SIAbsTree::Node(std::to_string(ifnode.second));
                                auto proto = new SIProto::IfElse(ifnode.second, loc, loc2);
                                proto->apply_offset(this->reader->line_number);
                                ifelsenode->assign_val(proto, type_ifelse);
                                this->Heapy->insertNode(ifelsenode);
                            } else {
                                auto K = this->Heapy->getNode(pname);
                                if (!K) {
                                    auto procnode = new SIAbsTree::Node(pname);
                                    procnode->assign_val(new SIProto::Proc(loc, loc2, this->reader->line_number), type_proc);
                                    this->Heapy->insertNode(procnode);
                                } else
                                    K->assign_val(new SIProto::Proc(loc, loc2, this->reader->line_number), type_proc);
                            }
                            total_startp--;
                            startp.pop_back();
                            continue;
                        }
                        this->ErrorLog("Unexpected <end>.");
                        return;
                    }
                }
            }
            if (total_startp > 0) {
                std::string t = startp[total_startp-1].first;
                if (t == "")
                    this->ErrorLog("Expected <end> near <if>.");
                else if (t == " ")
                    this->ErrorLog("Expected <end> near <else>.");
                else
                    this->ErrorLog("Expected <end> near '" + t + "' (<procedure>).");
                return;
            }
            this->_proto_init_ = true;
            this->reader->flush();
        }

        inline void* SIVM_CopyValue(void *val, const std::string &val_type) {
            if (val_type == type_str)
                return new std::string(*(std::string*)val);
            if (val_type == type_bool)
                return new bool(&*(bool*)val);
            if (val_type == type_num)
                return new double(*(double*)val);
            if (val_type == type_array)
                return new std::vector<SIStack::Val*>(*(std::vector<SIStack::Val*>*)val);
            return nullptr;
        }

        void SIVM_Exec(SIProto::Proc *main_proc) {
            std::stack<std::pair<std::vector<_SI_ULL>, _SI_ULL>> poses;
            poses.push(std::pair<std::vector<_SI_ULL>, _SI_ULL>(main_proc->get_loc(), 1));
            std::vector<_SI_ULL> curpos = poses.top().first;
            this->reader->new_region(curpos[0], curpos[1]);
            this->line_offset = 1;
            while (this->_feeded_ && this->_proto_init_) {
                this->nextToken();
                if (this->reader->getToken() == tk_eof) {
                    poses.pop();
                    if (!poses.empty()) {
                        curpos = poses.top().first;
                        this->line_offset = poses.top().second;
                        this->reader->new_region(curpos[0], curpos[1]);
                        continue;
                    }
                    break;
                }
                switch (this->reader->getToken())
                {
                    case tk_failure: //===< Error Token >===
                        this->ErrorLog(this->reader->getStrVal());
                        return;
                    case tk_number: //===< Number Token >===
                        this->Stacky->push(new SIStack::Val(type_num, new double(this->reader->getNumVal())));
                        break;
                    case tk_str: //===< String Token >===
                        this->Stacky->push(new SIStack::Val(type_str, new std::string(this->reader->getStrVal())));
                        break;
                    case tk_bool: //===< Boolean Token >===
                        this->Stacky->push(new SIStack::Val(type_bool, new bool(this->reader->getStrVal() == "true")));
                        break;
                    case tk_identifier: //===< Identifier Token >===
                    {
                        std::string ref_name = this->reader->getStrVal();
                        this->Stacky->push(new SIStack::Val(type_identifier, new std::string(ref_name)));
                        _SI_ULL org_pos = this->reader->current_read_loc();
                        this->nextToken();
                        std::string whatsnext = this->reader->getStrVal();
                        this->reader->new_region(org_pos, curpos[1]);
                        if (whatsnext == "ref" || whatsnext == "dref" || whatsnext == "jmp") {
                            break;
                        }
                        auto v = this->Heapy->getNode(ref_name);
                        if (!v)
                        {
                            this->ErrorLog_UNKNOWNREF(ref_name);
                            return;
                        }
                        this->Stacky->pop();
                        if (v->get_type() == type_proc) {
                            std::string proc_name = ref_name;
                            auto target_proc = this->Heapy->getNode(proc_name);
                            if (!target_proc)
                            {
                                this->ErrorLog("Unknown procedure's name: '" + proc_name + "'.");
                                return;
                            }
                            SIProto::Proc *tmp = (SIProto::Proc *)target_proc->get_val();
                            poses.push(std::pair<std::vector<_SI_ULL>,_SI_ULL>(tmp->get_loc(), tmp->get_offset()));
                            curpos = poses.top().first;
                            this->reader->new_region(curpos[0], curpos[1]);
                            this->line_offset = poses.top().second;
                        }
                        else
                            this->Stacky->push(new SIStack::Val(v->get_type(), this->SIVM_CopyValue(v->get_val(), v->get_type())));
                        break;
                    }
                    case tk_kword: //===< Keyword Token >===
                    {
                        std::string what = this->reader->getStrVal();

                        //####################################
                        //#        If&Else operators         #
                        //####################################
                        if (what == "if") {
                            if (!this->Stacky->empty()) {
                                auto top = this->Stacky->top();
                                if (top->get_type() != type_bool) {
                                    this->ErrorLog_EXPECTEDVAL(type_bool, top->get_type());
                                    return;
                                }
                                bool res = *(bool*)top->get_val();
                                //this->Stacky->pop();
                                auto scopy = this->Heapy->getNode(std::to_string(this->reader->current_read_loc()));
                                if (scopy) {
                                    SIProto::IfElse *proto = (SIProto::IfElse*) scopy->get_val();
                                    auto locs = proto->get_locs();
                                    _SI_ULL tmp = curpos[1];
                                    if (res) {
                                        poses.pop();
                                        poses.push(std::pair<std::vector<_SI_ULL>, _SI_ULL>({locs[2]+3, tmp}, proto->get_offset()));
                                        if (!locs[1])
                                            poses.push(std::pair<std::vector<_SI_ULL>, _SI_ULL>({locs[0], locs[2]}, proto->get_offset()));
                                        else
                                            poses.push(std::pair<std::vector<_SI_ULL>, _SI_ULL>({locs[0], locs[1]-4}, proto->get_offset()));
                                        curpos = poses.top().first;
                                        this->reader->new_region(curpos[0], curpos[1]);
                                        this->line_offset = poses.top().second;                                   
                                    } else {
                                        poses.pop();
                                        poses.push(std::pair<std::vector<_SI_ULL>, _SI_ULL>({locs[2]+4, tmp}, proto->get_offset()));
                                        if (locs[1])
                                            poses.push(std::pair<std::vector<_SI_ULL>, _SI_ULL>({locs[1], locs[2]}, proto->get_offset()));
                                        curpos = poses.top().first;
                                        this->reader->new_region(curpos[0], curpos[1]);
                                        this->line_offset = poses.top().second;
                                    }
                                }
                                break;
                            }
                            this->ErrorLog_STACKEMPTY();
                            return;
                        }


                        //####################################
                        //#         Logic operators          #
                        //####################################
                        if (what == "and" || what == "or") {
                            if (this->Stacky->size() > 1) {
                                std::vector<bool> sides;
                                for (int i = 0; i < 1; i++) {
                                    auto top = this->Stacky->top();
                                    if (top->get_type() == type_num)
                                        sides.push_back(*(double*)top->get_val() != 0);
                                    else if (top->get_type() == type_bool)
                                        sides.push_back(*(bool*)top->get_val());
                                    else if (top->get_type() == type_array)
                                        sides.push_back((*(std::vector<SIStack::Val*>*)top->get_val()).size() != 0);
                                    else if (top->get_type() == type_str)
                                        sides.push_back((*(std::string*)top->get_val()).length() != 0);
                                    this->Stacky->pop();
                                }
                                if (what == "and")
                                    this->Stacky->push(new SIStack::Val(type_bool, new bool(sides[0] == true && sides[1] == true)));
                                else
                                    this->Stacky->push(new SIStack::Val(type_bool, new bool(sides[0] == true || sides[1] == true)));
                                break;
                            }
                            this->ErrorLog_STACKEMPTY();
                            return;
                        }
                        if (what == "not") {
                            if (!this->Stacky->empty()) {
                                auto top = this->Stacky->top();
                                void *tmp0 = this->SIVM_CopyValue(top->get_val(), top->get_type());
                                std::string tmp1 = top->get_type();
                                this->Stacky->pop();
                                if (tmp1 == type_num)
                                    this->Stacky->push(new SIStack::Val(type_bool, new bool(*(double*)tmp0 == 0)));
                                else if (tmp1 == type_bool)
                                    this->Stacky->push(new SIStack::Val(type_bool, new bool(not *(bool*)tmp0)));
                                else if (tmp1 == type_array)
                                    this->Stacky->push(new SIStack::Val(type_bool, new bool((*(std::vector<SIStack::Val*>*)tmp0).size() == 0)));
                                else if (tmp1 == type_str)
                                    this->Stacky->push(new SIStack::Val(type_bool, new bool((*(std::string*)tmp0).length() == 0)));
                                break;
                            }
                            this->ErrorLog_STACKEMPTY();
                            return;
                        }


                        //####################################
                        //#         Array operators          #
                        //####################################
                        if (what == "mkarr") {
                            if (!this->Stacky->empty()) {
                                auto top = this->Stacky->top();
                                if (top->get_type() != type_num) {
                                    this->ErrorLog_EXPECTEDVAL(type_num, top->get_type());
                                    return;
                                }
                                double tmp = *(double*)top->get_val();
                                _SI_ULL arrsize = floor(tmp);
                                if (tmp < 0 || tmp != arrsize) {
                                    this->ErrorLog("Invalid size of array: " + std::to_string(tmp));
                                    return;
                                }
                                this->Stacky->pop();
                                if (this->Stacky->size() < arrsize) {
                                    this->ErrorLog("Cannot create new array with size of " + std::to_string(arrsize));
                                    return;
                                }
                                std::vector<SIStack::Val*> ar;
                                for (_SI_ULL i = arrsize; i > 0; i--) {
                                    top = this->Stacky->top();
                                    ar.push_back(new SIStack::Val(top->get_type(), this->SIVM_CopyValue(top->get_val(), top->get_type())));
                                    this->Stacky->pop();
                                }
                                this->Stacky->push(new SIStack::Val(type_array, new std::vector<SIStack::Val*>(ar)));
                                break;
                            }
                            this->ErrorLog_STACKEMPTY();
                            return;
                        }
                        if (what == "arrat") {
                            if (this->Stacky->size() > 1) {
                                SIStack::Val *top = this->Stacky->top();
                                if (top->get_type() != type_num) {
                                    this->ErrorLog_EXPECTEDVAL(type_num, top->get_type());
                                    return;
                                }
                                double tmp = *(double*)top->get_val();
                                _SI_ULL true_pos = floor(tmp);
                                if (tmp != true_pos || tmp < 0) {
                                    this->ErrorLog("Invalid array index: " + std::to_string(tmp));
                                    return;
                                }
                                this->Stacky->pop();
                                top = this->Stacky->top();
                                if (top->get_type() != type_array) {
                                    this->ErrorLog_EXPECTEDVAL(type_array, top->get_type());
                                    return;
                                }
                                std::vector<SIStack::Val*> d(*(std::vector<SIStack::Val*>*)top->get_val());
                                if (true_pos >= d.size()) {
                                    this->ErrorLog("Array index out of bound: " + std::to_string(true_pos));
                                    return;
                                }
                                this->Stacky->push(new SIStack::Val(type_num, new double(tmp)));
                                this->Stacky->push(new SIStack::Val(d.at(true_pos)->get_type(), d.at(true_pos)->get_val()));
                                break;
                            }
                            this->ErrorLog_STACKEMPTY();
                            return;
                        }
                        if (what == "arrconcat") {
                            if (this->Stacky->size() > 1) {
                                std::vector<std::vector<SIStack::Val*>> s;
                                for (int i = 0; i < 2; i++) {
                                    SIStack::Val *top = this->Stacky->top();
                                    if (top->get_type() != type_array) {
                                        this->ErrorLog_EXPECTEDVAL(type_array, top->get_type());
                                        return;
                                    }
                                    s.push_back(*(std::vector<SIStack::Val*>*)top->get_val());
                                    this->Stacky->pop();
                                }
                                s[0].insert(s[0].end(), s[1].begin(), s[1].end());
                                this->Stacky->push(new SIStack::Val(type_array, new std::vector<SIStack::Val*>(s[0])));
                                break;
                            }
                            this->ErrorLog_STACKEMPTY();
                            return;
                        }
                        if (what == "arrpush") {
                            if (this->Stacky->size() > 1) {
                                auto top = this->Stacky->top();
                                auto topush = new SIStack::Val(top->get_type(), this->SIVM_CopyValue(top->get_val(), top->get_type()));
                                this->Stacky->pop();
                                top = this->Stacky->top();
                                if (top->get_type() != type_array) {
                                    this->ErrorLog_EXPECTEDVAL(type_array, top->get_type());
                                    break;
                                }
                                ((std::vector<SIStack::Val*>*)top->get_val())->push_back(topush);
                                break;
                            }
                            this->ErrorLog_STACKEMPTY();
                            return;
                        }
                        if (what == "arrpop") {
                            if (!this->Stacky->empty()) {
                                SIStack::Val *top = this->Stacky->top();
                                if (top->get_type() != type_array) {
                                    this->ErrorLog_EXPECTEDVAL(type_array, top->get_type());
                                    return;
                                }
                                ((std::vector<SIStack::Val*>*)top->get_val())->pop_back();
                                break;
                            }
                            this->ErrorLog_STACKEMPTY();
                            return;
                        }


                        //####################################
                        //#         String operators         #
                        //####################################
                        //===< Concatenate strings >===
                        if (what == "strconcat") {
                            if (this->Stacky->size() > 1) {
                                std::string s[2];
                                for (int i = 0; i < 2; i++) {
                                    SIStack::Val *top = this->Stacky->top();
                                    if (top->get_type() != type_str) {
                                        this->ErrorLog_EXPECTEDVAL(type_str, top->get_type());
                                        return;
                                    }
                                    s[i] = *(std::string*)top->get_val();
                                    this->Stacky->pop();
                                }
                                this->Stacky->push(new SIStack::Val(type_str, new std::string(s[0]+s[1])));
                                break;
                            }
                            this->ErrorLog_STACKEMPTY();
                            return;
                        }
                        //===< Get chararacter at a specified index of string >===
                        if (what == "strat") {
                            if (this->Stacky->size() > 1)
                            {
                                SIStack::Val *top = this->Stacky->top();
                                if (top->get_type() != type_str)
                                {
                                    this->ErrorLog_EXPECTEDVAL(type_str, top->get_type());
                                    return;
                                }
                                std::string the_str = *(std::string *)top->get_val();
                                this->Stacky->pop();
                                top = this->Stacky->top();
                                double pos;
                                if (top->get_type() != type_num)
                                {
                                    this->ErrorLog_EXPECTEDVAL(type_num, top->get_type());
                                    return;
                                }
                                pos = *(double*)top->get_val();
                                _SI_ULL true_pos = floor(pos);
                                if (pos < 0 || pos != true_pos) {
                                    this->ErrorLog("Invalid string index: " + std::to_string(pos));
                                    return;
                                }
                                if (pos >= the_str.length()) {
                                    this->ErrorLog("String index out of range: " + std::to_string(pos));
                                    return;
                                }
                                this->Stacky->push(new SIStack::Val(type_str, new std::string(the_str.substr(true_pos,1))));
                                break;
                            }
                            this->ErrorLog_STACKEMPTY();
                            return;
                        }


                        //####################################
                        //#          Stack operators         #
                        //####################################
                        //===< Duplicate top of stack >===
                        if (what == "dup") {
                            if (!this->Stacky->empty()) {
                                auto top = this->Stacky->top();
                                this->Stacky->push(new SIStack::Val(top->get_type(), this->SIVM_CopyValue(top->get_val(), top->get_type())));
                                break;
                            }
                            this->ErrorLog_STACKEMPTY();
                            return;
                        }
                        //===< Rotate stack >===
                        if (what == "rotate") {
                            if (!this->Stacky->empty()) {
                                const _SI_ULL h = this->Stacky->size();
                                std::vector<SIStack::Val*> s;
                                _SI_ULL i;
                                for (i = h; i > 0; i--) {
                                    auto top = this->Stacky->top();
                                    s.push_back(new SIStack::Val(top->get_type(), this->SIVM_CopyValue(top->get_val(), top->get_type())));
                                    this->Stacky->pop();
                                }
                                for (i = 0; i < h; i++) {
                                    this->Stacky->push(new SIStack::Val(s[i]->get_type(), this->SIVM_CopyValue(s[i]->get_val(), s[i]->get_type())));
                                    delete(s[i]);
                                }
                                break;
                            }
                            this->ErrorLog_STACKEMPTY();
                            return;
                        }
                        //===< Swap top node with previous of it >===
                        if (what == "swap") {
                            if (this->Stacky->size() > 1) {
                                auto tmp = this->Stacky->top();
                                this->Stacky->pop();
                                std::swap(tmp, this->Stacky->top());
                                this->Stacky->push(tmp);
                                break;
                            }
                            this->ErrorLog_STACKEMPTY();
                            return;   
                        }
                        //===< Pop top of stack >===
                        if (what == "pop") {
                            if (!this->Stacky->empty()) {
                                this->Stacky->pop();
                                break;
                            }
                            this->ErrorLog_STACKEMPTY();
                            return;
                        }


                        //####################################
                        //#            Printing              #
                        //####################################
                        //===< Print to stdin the top value of stack >===
                        if (what == "print" || what == "println") {
                            if (!this->Stacky->empty()) {
                                std::string output_str;
                                auto top = this->Stacky->top();
                                //std::cout << "Output: ";
                                auto t = top->get_type();
                                auto v = top->get_val();
                                if (t == type_num)
                                    std::cout << *(double *)v;
                                else if (t == type_str)
                                    std::cout << *(std::string *)v;
                                else if (t == type_bool)
                                    std::cout << (*(bool*)v?"true":"false");
                                else if (t == type_array)
                                    std::cout << "Array at " << v;
                                if (what == "println")
                                    std::cout << "\n";
                                break;
                            }
                            this->ErrorLog_STACKEMPTY();
                            return;
                        }


                        //####################################
                        //#             Jumping              #
                        //####################################
                        //===< Jump to a procedure >===
                        if (what == "jmp") {
                            if (!this->Stacky->empty()) {
                                std::string proc_name;
                                auto top = this->Stacky->top();
                                if (top->get_type() != type_identifier)
                                {
                                    this->ErrorLog_EXPECTEDVAL(type_identifier, top->get_type());
                                    return;
                                }
                                proc_name = *(std::string *)top->get_val();
                                this->Stacky->pop();

                                auto target_proc = this->Heapy->getNode(proc_name);
                                if (!target_proc)
                                {
                                    this->ErrorLog("Unknown procedure's name: '" + proc_name + "'.");
                                    return;
                                }
                                SIProto::Proc *tmp = (SIProto::Proc *)target_proc->get_val();
                                poses.top().first[0] = this->reader->current_read_loc();
                                poses.push(std::pair<std::vector<_SI_ULL>, _SI_ULL>(tmp->get_loc(), tmp->get_offset()));
                                curpos = poses.top().first;
                                this->reader->new_region(curpos[0], curpos[1]);
                                this->line_offset = poses.top().second;
                                break;
                            }
                            this->ErrorLog_STACKEMPTY();
                            return;
                        }


                        //####################################
                        //#          (De)Reference           #
                        //####################################
                        if (what == "ref")
                        {
                            if (this->Stacky->size() > 1)
                            {
                                std::string name;
                                auto top = this->Stacky->top();
                                if (top->get_type() != type_identifier)
                                {
                                    this->ErrorLog_EXPECTEDVAL(type_identifier, top->get_type());
                                    return;
                                }
                                name = *(std::string *)top->get_val();
                                this->Stacky->pop();
                                top = this->Stacky->top();
                                auto K = this->Heapy->getNode(name);
                                if (!K) {
                                    auto u = new SIAbsTree::Node(name);
                                    u->assign_val(this->SIVM_CopyValue(top->get_val(), top->get_type()), top->get_type());
                                    this->Heapy->insertNode(u);
                                }
                                else {
                                    if (K->get_type() == type_proc) {
                                        this->ErrorLog("Cannot use procedure's name for refering.");
                                        return;
                                    }
                                    K->assign_val(this->SIVM_CopyValue(top->get_val(), top->get_type()), top->get_type());
                                }
                                this->Stacky->pop();
                                break;
                            }
                            this->ErrorLog_STACKEMPTY();
                            return;
                        }
                        if (what == "dref")
                        {
                            if (!this->Stacky->empty())
                            {
                                std::string name;
                                auto top = this->Stacky->top();
                                if (top->get_type() != type_identifier)
                                {
                                    this->ErrorLog_EXPECTEDVAL(type_identifier, top->get_type());
                                    return;
                                }
                                name = *(std::string *)top->get_val();
                                this->Stacky->pop();
                                auto v = this->Heapy->getNode(name);
                                if (!v)
                                {
                                    this->ErrorLog_UNKNOWNREF(name);
                                    return;
                                }
                                if (v->get_type() == type_proc) {
                                    this->ErrorLog("Cannot dereference a procedure.");
                                    return;
                                }
                                this->Heapy->delNode(v);
                                break;
                            }
                            this->ErrorLog_STACKEMPTY();
                            return;
                        }


                        //####################################
                        //#       Arithmetic operators       #
                        //####################################
                        //===< Basic arithmetic operators >==
                        if (what == "add" || what == "sub" || what == "mul" || what == "div" || what == "mod")
                        {
                            if (this->Stacky->size() > 1)
                            {
                                double v[2];
                                for (int i = 0; i < 2; i++)
                                {
                                    auto t = this->Stacky->top();
                                    if (t->get_type() != type_num)
                                    {
                                        this->ErrorLog_EXPECTEDVAL(type_num, t->get_type());
                                        return;
                                    }
                                    v[i] = *(double *)t->get_val();
                                    this->Stacky->pop();
                                }
                                if (v[0] == 0 && (what == "mod" || what == "div"))
                                {
                                    this->ErrorLog("Cannot divide by zero.");
                                    return;
                                }
                                if (what == "add")
                                    this->Stacky->push(new SIStack::Val(type_num, new double(v[1] + v[0])));
                                else if (what == "sub")
                                    this->Stacky->push(new SIStack::Val(type_num, new double(v[1] - v[0])));
                                else if (what == "mul")
                                    this->Stacky->push(new SIStack::Val(type_num, new double(v[1] * v[0])));
                                else if (what == "mod")
                                    this->Stacky->push(new SIStack::Val(type_num, new double(std::fmod(v[1], v[0]))));
                                else if (what == "div")
                                    this->Stacky->push(new SIStack::Val(type_num, new double(v[1] / v[0])));
                                break;
                            }
                            this->ErrorLog_STACKEMPTY();
                            return;
                        }

                        //####################################
                        //#       Inequality operators       #
                        //####################################
                        if (what == "gt" || what == "lt" || what == "gteq" || what == "lteq")
                        {
                            if (this->Stacky->size() > 1)
                            {
                                double v[2];
                                for (int i = 0; i < 2; i++)
                                {
                                    auto t = this->Stacky->top();
                                    if (t->get_type() != type_num)
                                    {
                                        this->ErrorLog_EXPECTEDVAL(type_num, t->get_type());
                                        return;
                                    }
                                    v[i] = *(double *)t->get_val();
                                    this->Stacky->pop();
                                }
                                if (what == "gt")
                                    this->Stacky->push(new SIStack::Val(type_bool, new bool(v[1] > v[0])));
                                else if (what == "lt")
                                    this->Stacky->push(new SIStack::Val(type_bool, new bool(v[1] < v[0])));
                                else if (what == "gteq")
                                    this->Stacky->push(new SIStack::Val(type_bool, new bool(v[1] >= v[0])));
                                else if (what == "lteq")
                                    this->Stacky->push(new SIStack::Val(type_bool, new bool(v[1] <= v[0])));
                                break;
                            }
                            this->ErrorLog_STACKEMPTY();
                            return;
                        }

                        //####################################
                        //#       Equality operators         #
                        //####################################
                        if (what == "eq" || what == "neq")
                        {
                            if (this->Stacky->size() > 1)
                            {
                                void *v[2];
                                std::string q[2];
                                for (int i = 0; i < 2; i++)
                                {
                                    auto t = this->Stacky->top();
                                    v[i] = SIVM_CopyValue(t->get_val(), t->get_type());
                                    q[i] = t->get_type();
                                    this->Stacky->pop();
                                }
                                if (q[0] == q[1])
                                {
                                    bool res = false;
                                    if (q[0] == type_str)
                                        res = (*(std::string *)v[0] == *(std::string *)v[1]);
                                    else if (q[0] == type_num)
                                        res = (*(double *)v[0] == *(double *)v[1]);
                                    else if (q[0] == type_bool)
                                        res = (*(bool *)v[0] == *(bool *)v[1]);
                                    else if (q[0] == type_array)
                                        res = (*(std::vector<SIStack::Val*>*)v[0] == *(std::vector<SIStack::Val*>*)v[1]);
                                    if (what == "neq")
                                        res = not res;
                                    this->Stacky->push(new SIStack::Val(type_bool, &res));
                                    free(v[0]);
                                    free(v[1]);
                                    break;
                                }
                                free(v[0]);
                                free(v[1]);
                                this->Stacky->push(new SIStack::Val(type_bool, new bool(what == "neq")));
                                break;
                            }
                            this->ErrorLog_STACKEMPTY();
                            return;
                        }
                    }
                }
            }
        }

    public:
        inline void feed(const std::string &si_input)
        {
            try {
                this->reader->change_story(si_input);
                this->line_offset = 1;
                this->si_buf = si_input;
                this->reader->new_region(0, si_input.length());
                this->_feeded_ = true;
            } catch (std::bad_alloc const &){
                this->ErrorLog("Not enough memory to initialize VM.");
            }
        };
        bool exec()
        {
            if (!this->_feeded_)
            {
                std::cout << "ERROR: VM isn't initialized.\n";
                return 1;
            }
            try
            {
                this->Proto_Initialize();
                auto main_proc = this->Heapy->getNode("main");
                if (!main_proc || this->reader->getToken() == tk_failure) {
                    this->_proto_init_ = false;
                    if (this->reader->getToken() != tk_failure)
                        this->ErrorLog("Expected a main procedure.");
                }
                else
                    this->SIVM_Exec((SIProto::Proc*)main_proc->get_val());
                this->si_buf = "";
                return 0;
            }
            catch (std::bad_alloc const &)
            {
                this->ErrorLog_NOMEM();
                return 1;
            }
        };
};

#endif