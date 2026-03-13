#include "CoolCodegen.h"
#include <iostream>
#include <fstream>
#include <unordered_map>
#include "../../include/codegen/CodeEmitter.h"
#include <cassert>

using namespace std;

int CoolCodegen::emitted_string_length(const string& s){
    int len = 0;
    for (size_t i = 0; i < s.size();){
        if (s[i] == '\\' && i + 1 < s.size()){
            i += 2;
            len += 1;
            continue;
        }
        i += 1;
        len += 1;
    }
    return len;
}

void CoolCodegen::build_class(int idx/*, vector<bool> &built_classes,
                 vector<unordered_map<string, mem_place>> &class_attr_locations,
                 vector<string> &dispatch_table_labels,
                 vector<int> &class_sizes*/) {
    int parent_idx = class_table_->get_parent_index(idx);
    if (parent_idx != -1 && !built_classes[parent_idx])
        //build_class(parent_idx, built_classes, class_attr_locations, dispatch_table_labels, class_sizes);
        build_class(parent_idx);
    // build class idx
    const vector<string> &all_attr_names = class_table_->get_all_attributes(idx);
    int offset = 12; // class_tag, obj_size, disp_table * 4(word size)
    for (size_t i = 0; i < all_attr_names.size(); ++i, offset +=4){
        const string &attr_name = all_attr_names[i];
        class_attr_locations[idx][attr_name] = mem_place(offset, true);
    }
    string class_name = string(class_table_->get_name(idx));
    dispatch_table_labels[idx] = class_name + "_dispTab";
    if (class_name == "Int") offset += 4; // int value
    else if (class_name == "Bool") offset += 4; // bool value
    else if (class_name == "String") {
        offset += 4; // string length
        offset += 1; // at least 1 byte (terminator) for string content
    }
    class_sizes[idx] = offset / 4 + (offset % 4 == 0 ? 0 : 1);//attrs + class tag, obj_size, disp_table
    built_classes[idx] = true;
}

void CoolCodegen::generate_call_Object_copy(std::ostream &out){
    print_decr_stack_pointer_4(out);
    riscv_emit::emit_call(out, "Object.copy");
    only_incr_stack_pointer_pos_n(4);
}

void CoolCodegen::generate_call_init(std::ostream &out, string &class_name){
    print_decr_stack_pointer_4(out);
    riscv_emit::emit_call(out, class_name + "_init");
    only_incr_stack_pointer_pos_n(4);
}

void CoolCodegen::Object_init(ostream &out, string label){
    //in $a0: self object pointer
    out << "\n";
    riscv_emit::emit_globl(out, label);
    riscv_emit::emit_label(out, label);
    cerr << "label: " << label << "\n";
    generate_method_in(out);
    riscv_emit::emit_load_word(out, ArgumentRegister{0}, MemoryLocation{-4, FramePointer{}});
    generate_method_out(out);

}

void CoolCodegen::String_init(ostream &out){
    //in $a0: self object pointer
    out << "\n";
    riscv_emit::emit_globl(out, "String_init");
    riscv_emit::emit_label(out, "String_init");
    /*riscv_emit::emit_add(out, FramePointer{}, StackPointer{}, ZeroRegister{});
    riscv_emit::emit_store_word(out, ReturnAddress{}, MemoryLocation{0, StackPointer{}});
    riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);
    riscv_emit::emit_store_word(out, ArgumentRegister{0}, MemoryLocation{0, StackPointer{}});*/
    cerr << "Generating String_init method\n";
    generate_method_in(out);
    riscv_emit::emit_load_address(out, ArgumentRegister{0}, "Int_protObj");
    riscv_emit::emit_store_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
    generate_call_Object_copy(out);
    /*
    t1 = -4(fp)
    a0 -> 12(t1)
    */
    //fp restored by Object.copy
    riscv_emit::emit_load_word(out, TempRegister{1}, MemoryLocation{-4, FramePointer{}});
    riscv_emit::emit_store_word(out, ArgumentRegister{0}, MemoryLocation{12, TempRegister{1}});
    riscv_emit::emit_load_word(out, ArgumentRegister{0}, MemoryLocation{-4, FramePointer{}});
    /*riscv_emit::emit_load_word(out, ReturnAddress{}, MemoryLocation{0, FramePointer{}});
    riscv_emit::emit_load_word(out, FramePointer{}, MemoryLocation{4, FramePointer{}});
    riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, 16);*/
    generate_method_out(out);
    riscv_emit::emit_ident(out);
    
    
}

bool CoolCodegen::is_basic_class(const string& class_name){
    return class_name == "Object" ||
           class_name == "IO" ||
           class_name == "Int" ||
           class_name == "Bool" ||
           class_name == "String";
}

void CoolCodegen::print_incr_stack_pointer_4(ostream &out) {
    riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, 4);
    stack_pointer_pos += 4;
}

void CoolCodegen::print_incr_stack_pointer_12(ostream &out) {
    riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, 12);
    stack_pointer_pos += 12;
}

void CoolCodegen::print_incr_stack_pointer_n(ostream &out, int n) {
    assert(n % 4 == 0);
    riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, n);
    stack_pointer_pos += n;
}

void CoolCodegen::print_decr_stack_pointer_4(ostream &out) {
    riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);
    stack_pointer_pos -= 4;
}

void CoolCodegen::print_decr_stack_pointer_12(ostream &out) {
    riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -12);
    stack_pointer_pos -= 12;
}

void CoolCodegen::print_decr_stack_pointer_n(ostream &out, int n) {
    assert(n % 4 == 0);
    riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -n);
    stack_pointer_pos -= n;
}

void CoolCodegen::generate_method_arguments_set(ostream &out, unordered_map<string, mem_place> &attr_locations,
                                                vector<string> &args_names, vector<Expr*> &args_exprs,
                                                string &dispatch_class_name, string &method_name){
    //unordered_map<string, mem_place> &attr_locations = class_attr_locations[static_dispatch_type];
    cerr << "Preparing arguments for method " << dispatch_class_name << "." << method_name << "\n";
    int count_args_exprs = args_exprs.size();
    for (size_t i = 0; i < count_args_exprs; ++i){
        generate_expr_code(out, args_exprs[i]);
        riscv_emit::emit_store_word(out, ArgumentRegister{0}, MemoryLocation{0, StackPointer{}});
        print_decr_stack_pointer_4(out);
    }
        for (auto it = attr_locations.begin(); it != attr_locations.end(); ++it)
            local_var_locations[it->first].push(it->second);
        //vector<string> args_names = class_table_->get_argument_names(static_dispatch_type, method_name);
        int num_args = args_names.size();
        //cerr << "Static dispatch to " << dispatch_class_name << "." << method_name 
        //     << " with " << num_args << " arguments.\n";
        for (size_t i = 0; i < num_args; ++i)
            //0(fp)->ra, 4(fp)->argN, ..., 4 + 4N(fp)->arg1, 8 + 4N(fp)->old fp
            local_var_locations[args_names[i]].push(mem_place((num_args  - i) * 4, false));
        //vector<Expr*> args_exprs = st_dispatch->get_arguments();
        
        riscv_emit::emit_comment(out, "load static dispatch target object into a0");
        riscv_emit::emit_load_word(out, ArgumentRegister{0}, MemoryLocation{4 * (count_args_exprs + 2), StackPointer{}});
        riscv_emit::emit_comment(out, "Calling method "   + method_name);
        int dispatch_type = class_table_->get_index(dispatch_class_name);
        int method_index = class_table_->get_method_index(dispatch_type, method_name);
        assert(method_index != -1);
        riscv_emit::emit_comment(out, "method from the dispatch table of the target object");
        riscv_emit::emit_load_word(out, TempRegister{1}, MemoryLocation{8, ArgumentRegister{0}});//load dispatch table pointer
        riscv_emit::emit_load_word(out, TempRegister{1}, MemoryLocation{4 * method_index, TempRegister{1}});//load method address
        //riscv_emit::emit_jump_and_link_register(out, TempRegister{1});
        riscv_emit::emit_ident(out);
        out << "jalr t1\n";
        //clean up arguments from the stack. target and fp cleanup is done in the caller generator
        only_incr_stack_pointer_pos_n(4 * (count_args_exprs));
        for (auto it = attr_locations.begin(); it != attr_locations.end(); ++it){
            assert(local_var_locations.find(it->first) != local_var_locations.end());
            assert(!local_var_locations[it->first].empty());
            assert(local_var_locations[it->first].top() == it->second);
            local_var_locations[it->first].pop();
        }
        for (size_t i = 0; i < args_names.size(); ++i){
            assert(local_var_locations.find(args_names[i]) != local_var_locations.end());
            assert(!local_var_locations[args_names[i]].empty());
            assert(local_var_locations[args_names[i]].top().offset == (int)((num_args  - i) * 4));
            local_var_locations[args_names[i]].pop();
        }   
}
void CoolCodegen::generate_static_dispatch_code(ostream &out, const StaticDispatch* st_dispatch){
    // TO BE IMPLEMENTED
        string method_name = st_dispatch->get_method_name();
        int static_dispatch_type = st_dispatch->get_static_dispatch_type();
        string dispatch_class_name = string(class_table_->get_name(static_dispatch_type));
        Expr* target = st_dispatch->get_target();//can be a declared variable, newly created object, attribute access, method call, constant, ...
        riscv_emit::emit_comment(out, "Static dispatch to " + dispatch_class_name + "." + method_name);
        riscv_emit::emit_comment(out, "Generating code for target object (res in a0)");
        generate_expr_code(out, target);
        riscv_emit::emit_comment(out, "Save the target object on the stack");
        riscv_emit::emit_store_word(out, ArgumentRegister{0}, MemoryLocation{0, StackPointer{}});
        print_decr_stack_pointer_4(out);
        riscv_emit::emit_branch_not_equal_zero(out, ArgumentRegister{0}, label_base + to_string(label_count));
        riscv_emit::emit_comment(out, "Runtime error: dispatch on void");
        string source_file_label = code_consts.string_consts[file_name_];
        riscv_emit::emit_load_address(out, ArgumentRegister{0}, source_file_label);
        int source_line_code = 0;
        riscv_emit::emit_add_immediate(out, TempRegister{1}, ZeroRegister{}, source_line_code);
        //riscv_emit::emit_jump_and_link(out, "_dispatch_abort");
        riscv_emit::emit_label(out, label_base + to_string(label_count));
        label_count++;
        riscv_emit::emit_store_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
        print_decr_stack_pointer_4(out);
        riscv_emit::emit_comment(out, "Preparing arguments for method call");
        unordered_map<string, mem_place> &attr_locations = class_attr_locations[static_dispatch_type];
        vector<string> args_names = class_table_->get_argument_names(static_dispatch_type, method_name);
        vector<Expr*> args_exprs = st_dispatch->get_arguments();
        generate_method_arguments_set(out, attr_locations, args_names, args_exprs, dispatch_class_name, method_name);
        only_incr_stack_pointer_pos_n(4);//the frame pointer "popped"
        riscv_emit::emit_comment(out, "remove from the stack the static dispatch target");
        
        //riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, 4);
        print_incr_stack_pointer_4(out);
}

void CoolCodegen:: generate_dynamic_dispatch_code(ostream &out, const DynamicDispatch* dy_dispatch){
    // TO BE IMPLEMENTED
    // TO BE IMPLEMENTED
        string method_name = dy_dispatch->get_method_name();
        Expr* target = dy_dispatch->get_target();//can be a declared variable, newly created object, attribute access, method call, constant, ...
        int dy_dispatch_type = target->get_type();
        string dispatch_class_name = string(class_table_->get_name(dy_dispatch_type));
        cerr << "Dynamic dispatch to " << dispatch_class_name << "." << method_name << "\n";
        riscv_emit::emit_comment(out, "dynamic dispatch to " + dispatch_class_name + "." + method_name);
        riscv_emit::emit_comment(out, "Generating code for target object (res in a0)");
        generate_expr_code(out, target);
        riscv_emit::emit_comment(out, "Save the target object on the stack");
        riscv_emit::emit_store_word(out, ArgumentRegister{0}, MemoryLocation{0, StackPointer{}});
        //riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);
        print_decr_stack_pointer_4(out);
        riscv_emit::emit_branch_not_equal_zero(out, ArgumentRegister{0}, label_base + to_string(label_count));
        riscv_emit::emit_comment(out, "Runtime error: dispatch on void");
        string source_file_label = code_consts.string_consts[file_name_];
        riscv_emit::emit_load_address(out, ArgumentRegister{0}, source_file_label);
        int source_line_code = 0;
        riscv_emit::emit_add_immediate(out, TempRegister{1}, ZeroRegister{}, source_line_code);
        //riscv_emit::emit_jump_and_link(out, "_dispatch_abort");
        riscv_emit::emit_label(out, label_base + to_string(label_count));
        label_count++;
        riscv_emit::emit_store_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
        //riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);
        print_decr_stack_pointer_4(out);
        riscv_emit::emit_comment(out, "Preparing arguments for method call");
        unordered_map<string, mem_place> &attr_locations = class_attr_locations[dy_dispatch_type];
        int method_index = class_table_->get_method_index(dy_dispatch_type, method_name);
        assert(method_index != -1);
        vector<pair<string, int>> dispatch_table = class_table_->get_all_methods(dy_dispatch_type);
        int method_class_index = dispatch_table[method_index].second;
        vector<string> args_names = class_table_->get_argument_names(method_class_index, method_name);
        vector<Expr*> args_exprs = dy_dispatch->get_arguments();
        string method_class_name = string(class_table_->get_name(method_class_index));
        generate_method_arguments_set(out, attr_locations, args_names, args_exprs, method_class_name, method_name);
        //clean up frame pointer from the stack
        only_incr_stack_pointer_pos_n(4);
        riscv_emit::emit_comment(out, "remove from the stack the dynamic dispatch target");
        //riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, 4);
        print_incr_stack_pointer_4(out);
}

void CoolCodegen::generate_method_invoke(ostream &out, const MethodInvocation* method_invocation){
    // TO BE IMPLEMENTED
    ///save the self object on the stack
    riscv_emit::emit_comment(out, "Save the self object on the stack");
    riscv_emit::emit_load_word(out, ArgumentRegister{0}, MemoryLocation{-4, FramePointer{}});
    riscv_emit::emit_store_word(out, ArgumentRegister{0}, MemoryLocation{0, StackPointer{}});
    //riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);
    print_decr_stack_pointer_4(out);
    riscv_emit::emit_comment(out, "save the frame pointer on the stack");
    riscv_emit::emit_store_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
    //riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);
    print_decr_stack_pointer_4(out);
    riscv_emit::emit_comment(out, "Preparing arguments for method call");
    int defining_class_index = in_class;
    string method_name = method_invocation->get_method_name();
    unordered_map<string, mem_place> &attr_locations = class_attr_locations[defining_class_index];
    assert(defining_class_index != -1);
    int method_index = class_table_->get_method_index(defining_class_index, method_name);
    assert(method_index != -1);
    vector<pair<string, int>> dispatch_table = class_table_->get_all_methods(defining_class_index);
    int method_class_index = dispatch_table[method_index].second;

    vector<string> args_names = class_table_->get_argument_names(method_class_index, method_name);
    vector<Expr*> args_exprs = method_invocation->get_arguments();
    string class_name = string(class_table_->get_name(method_class_index));
    
    generate_method_arguments_set(out, attr_locations, args_names, args_exprs, class_name, method_name);
    //clean up frame pointer from the stack
    only_incr_stack_pointer_pos_n(4);
    riscv_emit::emit_comment(out, "remove from the stack the self object for method invocation");
    //riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, 4);
    print_incr_stack_pointer_4(out);
}

void CoolCodegen::generate_expr_code(ostream &out, const Expr* expr){
    //static string label_base = "label_";
    // TO BE IMPLEMENTED
    //if (auto method_expr = dynamic_cast<const Method *>(expr)) never happens
    if (auto st_dispatch = dynamic_cast<const StaticDispatch *>(expr)){
        generate_static_dispatch_code(out, st_dispatch);
    }
    else if (auto dynamic_dispatch = dynamic_cast<const DynamicDispatch *>(expr)){
        generate_dynamic_dispatch_code(out, dynamic_dispatch);
        //int sth;
    }
    else if (auto method_invocation = dynamic_cast<const MethodInvocation *>(expr)){
        generate_method_invoke(out, method_invocation);
    }

    else if (auto str_const = dynamic_cast<const StringConstant *>(expr)){
        //cerr << "Found a string constant: " << str_const->get_value() << endl;
        string str_value = str_const->get_value();
        str_value = str_value.substr(1, str_value.length() - 2); // remove quotes
        code_consts.add_string(str_value);
        code_consts.add_int(emitted_string_length(str_value));
        string str_label = code_consts.string_consts[str_value];
        riscv_emit::emit_load_address(out, ArgumentRegister{0}, str_label);
        riscv_emit::emit_store_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
        //riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);
        generate_call_Object_copy(out);
    }
    else if (auto int_const = dynamic_cast<const IntConstant *>(expr)){
        int int_value = int_const->get_value();
        code_consts.add_int(int_value);
        string int_label = code_consts.int_consts[int_value];
        riscv_emit::emit_load_address(out, ArgumentRegister{0}, int_label);
        riscv_emit::emit_store_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
        //riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);
        generate_call_Object_copy(out);
    }
    else if (auto bool_const = dynamic_cast<const BoolConstant *>(expr)){
        bool bool_value = bool_const->get_value();
        code_consts.add_bool(bool_value);
        string bool_label = code_consts.bool_consts[bool_value];
        riscv_emit::emit_load_address(out, ArgumentRegister{0}, bool_label);
        riscv_emit::emit_store_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
        //riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);
        generate_call_Object_copy(out);
    }
    else if (auto objectid = dynamic_cast<const ObjectReference *>(expr)){
        string var_name = objectid->get_name();
        if (var_name == "self"){
            riscv_emit::emit_load_word(out, ArgumentRegister{0}, MemoryLocation{-4, FramePointer{}});
            return;
        }
        //return;
        //return;
        assert(local_var_locations.find(var_name) != local_var_locations.end());
        assert(!local_var_locations[var_name].empty());
        mem_place var_location = local_var_locations[var_name].top();
        cerr << "Accessing variable " << var_name << " with location offset " << var_location.offset 
             << (var_location.isattr ? " (attribute)" : " (local variable)") << "\n";
        cerr << "Control print: " << local_var_locations[var_name].size() << "\n";
        if (var_location.isattr){
            //load self object pointer from stack
            riscv_emit::emit_load_word(out, ArgumentRegister{0}, MemoryLocation{-4, FramePointer{}});
            riscv_emit::emit_load_word(out, ArgumentRegister{0}, MemoryLocation{var_location.offset, ArgumentRegister{0}});
        }
        else riscv_emit::emit_load_word(out, ArgumentRegister{0}, MemoryLocation{var_location.offset, FramePointer{}});
    }
    else if (auto assign = dynamic_cast<const Assignment *>(expr)){
        string assignee_name = assign->get_assignee_name();
        generate_expr_code(out, assign->get_value());//result in a0
        riscv_emit::emit_comment(out, "Storing assignment result into variable " + assignee_name);
        //riscv_emit::emit_store_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
        //riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);
        //riscv_emit::emit_jump_and_link(out, "Object.copy");
        assert(local_var_locations.find(assignee_name) != local_var_locations.end());
        mem_place assignee_location = local_var_locations[assignee_name].top();
        if (assignee_location.is_attr()){
            //load self object pointer from stack
            riscv_emit::emit_load_word(out, TempRegister{1}, MemoryLocation{-4, FramePointer{}});
            riscv_emit::emit_store_word(out, ArgumentRegister{0}, MemoryLocation{assignee_location.get_offset(), TempRegister{1}});
        }
        else riscv_emit::emit_store_word(out, ArgumentRegister{0}, MemoryLocation{assignee_location.get_offset(), FramePointer{}});
    }
    else if (auto new_expr = dynamic_cast<const NewObject *>(expr)){
        int type_index = new_expr->get_type();
        string class_name = string(class_table_->get_name(type_index));
        riscv_emit::emit_comment(out, "Creating new object of class " + class_name);
        riscv_emit::emit_load_address(out, ArgumentRegister{0}, class_name + "_protObj");
        //iscv_emit::emit_store_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
        //riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);
        //riscv_emit::emit_call(out, "Object.copy");
        riscv_emit::emit_store_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
        //riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);
        generate_call_init(out, class_name);
    }
    else if (auto sequence = dynamic_cast<const Sequence *>(expr)){
        const vector<Expr*> exprs = sequence->get_sequence();
        for (size_t i = 0; i < exprs.size(); ++i)
            generate_expr_code(out, exprs[i]);
    }
    else if (auto let_expr = dynamic_cast<const LetIn *>(expr)){
        vector<Vardecl *> var_decls = let_expr->get_vardecls();
        riscv_emit::emit_comment(out, "Let-in expression: allocating space for local variables (accessed via negative offsets from fp)");
        int num_vars = var_decls.size();
        //riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4 * num_vars);
        negative_offset_fp = stack_pointer_pos;
        print_decr_stack_pointer_n(out, 4 * num_vars);
        for (size_t i = 0; i < num_vars; ++i){
            Vardecl* var_decl = var_decls[i];
            string var_name = var_decl->get_name();
            //push local var on the stack but with negative offset from fp
            //push
            string var_type_name = string(class_table_->get_name(var_decl->get_type()));
            cerr << "Let-in variable " << var_name << " of type " << var_type_name << "with offset " << negative_offset_fp << " from fp\n";
            local_var_locations[var_name].push(mem_place(negative_offset_fp, false));
            cerr << "Control print: " << local_var_locations[var_name].top().offset << "\n";
            if (var_decl->has_initializer()){
                Expr* init_expr = var_decl->get_initializer();
                riscv_emit::emit_comment(out, "Let-in variable " + var_name + " initialization");
                generate_expr_code(out, init_expr);//result in a0
                riscv_emit::emit_store_word(out, ArgumentRegister{0}, MemoryLocation{negative_offset_fp, FramePointer{}});
            }
            else if (var_type_name == "Int"){
                riscv_emit::emit_comment(out, "Let-in variable " + var_name + " default initialization to 0");
                static string int_zero_label = code_consts.int_consts[0];
                riscv_emit::emit_load_address(out, ArgumentRegister{0}, int_zero_label);
                riscv_emit::emit_store_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
                //riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);
                generate_call_Object_copy(out);
                riscv_emit::emit_store_word(out, ArgumentRegister{0}, MemoryLocation{negative_offset_fp, FramePointer{}});
            }
            else if (var_type_name == "Bool"){
                riscv_emit::emit_comment(out, "Let-in variable " + var_name + " default initialization to false");
                static string bool_false_label = code_consts.bool_consts[0];
                riscv_emit::emit_load_address(out, ArgumentRegister{0}, bool_false_label);
                riscv_emit::emit_store_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
                //riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);
                generate_call_Object_copy(out);
                riscv_emit::emit_store_word(out, ArgumentRegister{0}, MemoryLocation{negative_offset_fp, FramePointer{}});
            }
            else if (var_type_name == "String"){
                riscv_emit::emit_comment(out, "Let-in variable " + var_name + " default initialization to empty string");
                static string string_empty_label = code_consts.string_consts[""];
                riscv_emit::emit_load_address(out, ArgumentRegister{0}, string_empty_label);
                riscv_emit::emit_store_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
                //riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);
                generate_call_Object_copy(out);
                riscv_emit::emit_store_word(out, ArgumentRegister{0}, MemoryLocation{negative_offset_fp, FramePointer{}});
            }
            else riscv_emit::emit_store_word(out, ZeroRegister{}, MemoryLocation{negative_offset_fp, FramePointer{}});
            negative_offset_fp -= 4;
        }
        riscv_emit::emit_comment(out, "Generating code for let-in body");
        generate_expr_code(out, let_expr->get_body());//result in a0
        //reduce stack and clean local_var_locations
        riscv_emit::emit_comment(out, "Let-in cleanup: deallocating space for local variables");
        //riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, 4 * num_vars);
        print_incr_stack_pointer_n(out, 4 * num_vars);
        negative_offset_fp += 4 * num_vars;
        for (size_t i = 0; i < num_vars; ++i){
            Vardecl* var_decl = var_decls[i];
            string var_name = var_decl->get_name();
            assert(local_var_locations.find(var_name) != local_var_locations.end());
            assert(!local_var_locations[var_name].empty());
            local_var_locations[var_name].pop();
        }
    }
    else if (auto vardecl = dynamic_cast<const Vardecl *>(expr)){int sth;}
    else if (auto if_expr = dynamic_cast<const IfThenElseFi *>(expr)){
        static string then_label_base = "then_label_";
        static string else_label_base = "else_label_";
        static string fi_label_base = "fi_label_";
        int current_label = label_count++;//recursive calls safe
        string curr_str = to_string(current_label);
        riscv_emit::emit_comment(out, "If-then-else expression: generating condition code");
        generate_expr_code(out, if_expr->get_condition());//result in a0
        //compare a0 with true
        riscv_emit::emit_load_word(out, TempRegister{1}, MemoryLocation{12, ArgumentRegister{0}});//load bool value
        riscv_emit::emit_branch_not_equal_zero(out, TempRegister{1}, then_label_base + curr_str);
        //else part
        riscv_emit::emit_label(out, else_label_base + curr_str);
        generate_expr_code(out, if_expr->get_else_expr());//result
        riscv_emit::emit_jump(out, fi_label_base + curr_str);
        //then part
        riscv_emit::emit_label(out, then_label_base + curr_str);
        generate_expr_code(out, if_expr->get_then_expr());//result
        //fi label
        riscv_emit::emit_label(out, fi_label_base + curr_str);
    }
    else if (auto isvoid_expr = dynamic_cast<const IsVoid *>(expr)){
        riscv_emit::emit_comment(out, "isvoid expression: generating code for sub-expression");
        generate_expr_code(out, isvoid_expr->get_subject());//result in a0
        //check if a0 == 0. correspondingly load true/false object into a0
        static string bool_true_label = code_consts.bool_consts[1];
        static string bool_false_label = code_consts.bool_consts[0];
        int current_label = label_count++;//recursive calls safe
        string curr_str = to_string(current_label);
        string label_true = "isvoid_true_" + curr_str;
        string label_false = "isvoid_false_" + curr_str;
        string label_end = "isvoid_end_" + curr_str;
        riscv_emit::emit_branch_equal_zero(out, ArgumentRegister{0}, label_true);
        //not void
        riscv_emit::emit_label(out, label_false);
        riscv_emit::emit_load_address(out, ArgumentRegister{0}, bool_false_label);
        riscv_emit::emit_store_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
        //riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);
        generate_call_Object_copy(out);
        riscv_emit::emit_jump(out, label_end);
        //is void
        riscv_emit::emit_label(out, label_true);
        riscv_emit::emit_load_address(out, ArgumentRegister{0}, bool_true_label);
        riscv_emit::emit_store_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
        //riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);
        generate_call_Object_copy(out);
        //end
        riscv_emit::emit_label(out, label_end);
    }
    else if (auto eq_cmp = dynamic_cast<const EqualityComparison *>(expr)){
        //first argument on the stack, second in a0
        //result in a0 (bool object)
        riscv_emit::emit_comment(out, "Equality comparison expression: generating code for left operand");
        riscv_emit::emit_store_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
        riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);
        generate_expr_code(out, eq_cmp->get_lhs());//result in a0
        riscv_emit::emit_comment(out, "Saving left operand result on the stack");
        riscv_emit::emit_store_word(out, ArgumentRegister{0}, MemoryLocation{0, StackPointer{}});
        riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);
        riscv_emit::emit_comment(out, "Generating code for right operand");
        generate_expr_code(out, eq_cmp->get_rhs());//result in a0
        //call _compare_equal
        riscv_emit::emit_comment(out, "Calling _compare_equal for equality comparison");
        /*riscv_emit::emit_store_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
        riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);*/
        riscv_emit::emit_jump_and_link(out, "_compare_equal");
        riscv_emit::emit_comment(out, "Equality comparison completed, result in a0");
    }
    else if (auto while_expr = dynamic_cast<const WhileLoopPool *>(expr)){
        static string loop_label_base = "loop_label_";
        static string end_loop_label_base = "end_loop_label_";
        int current_label = label_count++;//recursive calls safe
        string curr_str = to_string(current_label);
        riscv_emit::emit_label(out, loop_label_base + curr_str);
        riscv_emit::emit_comment(out, "While loop: generating condition code");
        generate_expr_code(out, while_expr->get_condition());//result in a0
        //compare a0 with true
        riscv_emit::emit_load_word(out, TempRegister{1}, MemoryLocation{12, ArgumentRegister{0}});//load bool value
        riscv_emit::emit_branch_equal_zero(out, TempRegister{1}, end_loop_label_base + curr_str);
        riscv_emit::emit_comment(out, "While loop: generating body code");
        generate_expr_code(out, while_expr->get_body());//result in a0
        riscv_emit::emit_jump(out, loop_label_base + curr_str);
        riscv_emit::emit_label(out, end_loop_label_base + curr_str);
    }
    else if (auto int_neg = dynamic_cast<const IntegerNegation *>(expr)){
        generate_expr_code(out, int_neg->get_argument());//result in a0
        riscv_emit::emit_comment(out, "Integer negation: negating integer value");
        riscv_emit::emit_load_word(out, TempRegister{1}, MemoryLocation{12, ArgumentRegister{0}});//load int value
        riscv_emit::emit_ident(out);
        out << "neg t1, t1\n";
        riscv_emit::emit_store_word(out, TempRegister{1}, MemoryLocation{12, ArgumentRegister{0}});//store back
        /*riscv_emit::emit_add_immediate(out, TempRegister{1}, ZeroRegister{}, 0);
        riscv_emit::emit_subtract(out, TempRegister{1}, ZeroRegister{}, TempRegister{1});//negate
        riscv_emit::emit_store_word(out, TempRegister{1}, MemoryLocation{12, ArgumentRegister{0}});//store back*/
    }
    else if (auto int_arith = dynamic_cast<const Arithmetic *>(expr)){
        /*
        first generate lhs into a0
        store lhs int value on the stack
        generate rhs into a0
        load lhs int value from the stack into t1
        Object.copy a0 to have a result object
        load rhs int value from a0 into t2
        perform operation between t1 and t2 int values
        store result back into a0
        */
        generate_expr_code(out, int_arith->get_lhs());//result in a0
        riscv_emit::emit_comment(out, "Saving left operand integer value on the stack");//saving the number not int object
        riscv_emit::emit_load_word(out, TempRegister{1}, MemoryLocation{12, ArgumentRegister{0}});//load int value
        riscv_emit::emit_store_word(out, TempRegister{1}, MemoryLocation{0, StackPointer{}});
        print_decr_stack_pointer_4(out);
        generate_expr_code(out, int_arith->get_rhs());//result in a0
        riscv_emit::emit_comment(out, "Creating result integer object for snd value via Object.copy");
        riscv_emit::emit_store_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
        generate_call_Object_copy(out);
        riscv_emit::emit_comment(out, "Loading left operand integer value from the stack");
        riscv_emit::emit_load_word(out, TempRegister{1}, MemoryLocation{4, StackPointer{}});
        riscv_emit::emit_comment(out, "Loading right operand integer value from a0");
        riscv_emit::emit_load_word(out, TempRegister{2}, MemoryLocation{12, ArgumentRegister{0}});//load rhs int value
        Arithmetic::Kind op = int_arith->get_kind();
        riscv_emit::emit_comment(out, "Performing arithmetic operation");
        switch (op){
            case Arithmetic::Kind::Addition:
                riscv_emit::emit_ident(out);
                out << "add t1, t1, t2\n";
                break;
            case Arithmetic::Kind::Subtraction:
                riscv_emit::emit_ident(out);
                out << "sub t1, t1, t2\n";
                break;
            case Arithmetic::Kind::Multiplication:
                riscv_emit::emit_ident(out);
                out << "mul t1, t1, t2\n";
                break;
            case Arithmetic::Kind::Division:
                riscv_emit::emit_ident(out);
                out << "div t1, t1, t2\n";
                break;
            default:
                assert(false && "Unknown arithmetic operation");
        }
        riscv_emit::emit_store_word(out, TempRegister{1}, MemoryLocation{12, ArgumentRegister{0}});//store back
        //pop from stack the saved lhs int value
        print_incr_stack_pointer_4(out);
    }
    else if (auto int_cmp = dynamic_cast < const IntegerComparison *>(expr)){
        /*
        first generate lhs into a0
        store lhs int value on the stack
        generate rhs into a0
        load rhs int value from a0 into t2
        load lhs int value from the stack into t1
        perform comparison between t1 and t2 int values
        store bool result back into a0(load true/false object in a0 in advance)
        */
        riscv_emit::emit_comment(out, "Integer comparison expression: generating code for left operand");
        generate_expr_code(out, int_cmp->get_lhs());//result in a0
        riscv_emit::emit_comment(out, "Saving left operand integer value on the stack");//saving the number not int object
        riscv_emit::emit_load_word(out, TempRegister{1}, MemoryLocation{12, ArgumentRegister{0}});//load int value
        riscv_emit::emit_store_word(out, TempRegister{1}, MemoryLocation{0, StackPointer{}});
        print_decr_stack_pointer_4(out);
        riscv_emit::emit_comment(out, "Generating code for right operand for integer comparison");
        generate_expr_code(out, int_cmp->get_rhs());//result in a0
        riscv_emit::emit_comment(out, "Loading right operand integer value from a0");
        riscv_emit::emit_load_word(out, TempRegister{2}, MemoryLocation{12, ArgumentRegister{0}});//load rhs int value
        riscv_emit::emit_comment(out, "Loading left operand integer value from the stack");
        riscv_emit::emit_load_word(out, TempRegister{1}, MemoryLocation{4, StackPointer{}});
        print_incr_stack_pointer_4(out);
        riscv_emit::emit_comment(out, "Performing integer comparison operation");
        IntegerComparison::Kind op = int_cmp->get_kind();
        switch (op){
            case IntegerComparison::Kind::LessThan:
                //riscv_emit::emit_branch_less_than(out, TempRegister{1}, TempRegister{2}, label_true);
                riscv_emit::emit_set_less_than(out, TempRegister{3}, TempRegister{1}, TempRegister{2});
                break;
            case IntegerComparison::Kind::LessThanEqual:
                //riscv_emit::emit_branch_less_equal(out, TempRegister{1}, TempRegister{2}, label_true);
                riscv_emit::emit_set_less_than(out, TempRegister{3}, TempRegister{2}, TempRegister{1});
                riscv_emit::emit_xor_immediate(out, TempRegister{3}, TempRegister{3}, 1);
                break;
            default:
                assert(false && "Unknown integer comparison operation");
        }
        int current_label = label_count++;//recursive calls safe
        string curr_str = to_string(current_label);
        string label_true = "int_cmp_true_" + curr_str;
        string label_false = "int_cmp_false_" + curr_str;
        string label_end = "int_cmp_end_" + curr_str;
        riscv_emit::emit_branch_not_equal_zero(out, TempRegister{3}, label_true);
        //false
        riscv_emit::emit_label(out, label_false);
        static string bool_false_label = code_consts.bool_consts[0];
        riscv_emit::emit_load_address(out, ArgumentRegister{0}, bool_false_label);
        riscv_emit::emit_store_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
        generate_call_Object_copy(out);
        riscv_emit::emit_jump(out, label_end);
        //true
        riscv_emit::emit_label(out, label_true);
        static string bool_true_label = code_consts.bool_consts[1];
        riscv_emit::emit_load_address(out, ArgumentRegister{0}, bool_true_label);
        riscv_emit::emit_store_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
        generate_call_Object_copy(out);
        //end
        riscv_emit::emit_label(out, label_end);
    }
    else if (auto bool_neg = dynamic_cast <const BooleanNegation *>(expr)){
        riscv_emit::emit_comment(out, "Boolean negation: generating code for argument");
        generate_expr_code(out, bool_neg->get_argument());//result in a0
        /*load bool value from a0
        xor with 1
        store back into a0*/
        riscv_emit::emit_load_word(out, TempRegister{1}, MemoryLocation{12, ArgumentRegister{0}});//load bool value
        riscv_emit::emit_ident(out);
        out << "xori t1, t1, 1\n";
        riscv_emit::emit_store_word(out, TempRegister{1}, MemoryLocation{12, ArgumentRegister{0}});//store back
    }
    else if (auto parenthesized_expr = dynamic_cast<const ParenthesizedExpr *>(expr)){
        generate_expr_code(out, parenthesized_expr->get_contents());
    }
    else if (auto case_expr = dynamic_cast<const CaseOfEsac *>(expr)){
        // TO BE IMPLEMENTED
        riscv_emit::emit_comment(out, "Case expression code generation to be implemented");
        riscv_emit::emit_comment(out, "Case expression: generating code for subject expression");
        generate_expr_code(out, case_expr->get_multiplex());//result in a0
        riscv_emit::emit_comment(out, "Check for void case expression subject");
        string case_label_base = "case_label_" + to_string(case_count);
        string case_end_label = "case_end_label_" + to_string(case_count);
        case_count++;
        riscv_emit::emit_branch_not_equal_zero(out, ArgumentRegister{0}, case_label_base);
        //void case
        riscv_emit::emit_comment(out, "Case expression runtime error: case on void");
        riscv_emit::emit_load_address(out, TempRegister{0}, code_consts.string_consts[file_name_]);
        riscv_emit::emit_store_word(out, TempRegister{0}, MemoryLocation{0, StackPointer{}});
        print_decr_stack_pointer_4(out);
        //riscv_emit::emit_load_immediate(out, TempRegister{1}, case_expr->get_line());
        //add the line number to the code constants
        code_consts.add_int(case_expr->get_line());
        string line_label = code_consts.int_consts[case_expr->get_line()];
        riscv_emit::emit_load_address(out, TempRegister{1}, line_label);
        riscv_emit::emit_store_word(out, TempRegister{1}, MemoryLocation{0, StackPointer{}});
        print_decr_stack_pointer_4(out);
        riscv_emit::emit_jump_and_link(out, "_case_abort_on_void");
        only_incr_stack_pointer_pos_n(8);
        riscv_emit::emit_label(out, case_label_base);
        //non-void case
        riscv_emit::emit_comment(out, "Store the case multiplex on the stack");
        //will use negative_offset_fp to allocate the multiplex
        negative_offset_fp = stack_pointer_pos;
        int curr_neg_offset_fp = negative_offset_fp;
        riscv_emit::emit_store_word(out, ArgumentRegister{0}, MemoryLocation{negative_offset_fp, FramePointer{}});
        print_decr_stack_pointer_4(out);
        const vector<CaseOfEsac::Case> & cases = case_expr->get_cases();
        vector<class_range> class_ranges;
        for (int i = 0; i < cases.size(); ++i){
            int var_type_index = cases[i].get_type();
            int sub_hier_sz = class_table_->get_sub_hierarchy_size(var_type_index);
            class_ranges.push_back(class_range(var_type_index, sub_hier_sz, i));
        }
        sort(class_ranges.begin(), class_ranges.end());//predifined operator<
        /*for (int i = 0; i < class_ranges.size(); ++i){
            const CaseOfEsac::Case & curr_case = cases[class_ranges[i].case_index];
        }*/
        
        for (int i = 0; i < cases.size(); ++i){
            //const CaseOfEsac::Case & curr_case = cases[i];
            const CaseOfEsac::Case & curr_case = cases[class_ranges[i].case_branch_index];            
            string var_name = curr_case.get_name();
            int var_type_index = curr_case.get_type();
            mem_place case_var_place(curr_neg_offset_fp, false);
            local_var_locations[var_name].push(case_var_place);
            riscv_emit::emit_comment(out, "Case branch for variable " + var_name + " of type " + string(class_table_->get_name(var_type_index)));
            //compare the class tags of the multiplex and the case type
            riscv_emit::emit_comment(out, "Loading class tag of case multiplex");
            riscv_emit::emit_load_word(out, TempRegister{1}, MemoryLocation{curr_neg_offset_fp, FramePointer{}});
            riscv_emit::emit_load_word(out, TempRegister{1}, MemoryLocation{0, TempRegister{1}});//load class tag
            riscv_emit::emit_comment(out, "Case class tag is known at compile time");
            //riscv_emit::emit_load_immediate(out, TempRegister{2}, var_type_index);
            /*riscv_emit::emit_ident(out);
            out << "li t2, " << var_type_index << "\n";
            int cnt_lab = label_count++;
            string curr_str = label_base + to_string(cnt_lab);
            out << "bne t1, t2, " << curr_str << "\n";*/
            //match found
            int cnt_lab = label_count++;
            string curr_str = label_base + to_string(cnt_lab);
            int l = class_ranges[i].class_index;
            int r = l + class_ranges[i].sub_hierarchy_size - 1;
            riscv_emit::emit_ident(out);
            out << "li t2, " << l << "\n";
            riscv_emit::emit_ident(out);
            out << "blt t1, t2, " << curr_str << "\n";
            riscv_emit::emit_ident(out);
            out << "li t2, " << r << "\n";
            riscv_emit::emit_ident(out);
            out << "bgt t1, t2, " << curr_str << "\n";
            riscv_emit::emit_comment(out, "Case branch match found: generating code for case body");
            generate_expr_code(out, curr_case.get_expr());//result in a0
            riscv_emit::emit_jump(out, case_end_label);
            //no match, continue
            riscv_emit::emit_label(out, curr_str);
            //clean up local_var_locations
            assert(local_var_locations.find(var_name) != local_var_locations.end());
            assert(!local_var_locations[var_name].empty());
            assert(local_var_locations[var_name].top() == case_var_place);
            local_var_locations[var_name].pop();
        }
        //no match found for any case call _case_abort_no_match
        riscv_emit::emit_comment(out, "Case expression runtime error: no matching branch found");
        riscv_emit::emit_store_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
        print_decr_stack_pointer_4(out);
        riscv_emit::emit_load_address(out, TempRegister{0}, code_consts.string_consts[file_name_]);
        riscv_emit::emit_store_word(out, TempRegister{0}, MemoryLocation{0, StackPointer{}});
        print_decr_stack_pointer_4(out);
        //riscv_emit::emit_load_immediate(out, TempRegister{1}, case_expr->get_line());
        riscv_emit::emit_load_address(out, TempRegister{1}, line_label);
        riscv_emit::emit_store_word(out, TempRegister{1}, MemoryLocation{0, StackPointer{}});
        print_decr_stack_pointer_4(out);
        //class_nameTab
        riscv_emit::emit_load_address(out, TempRegister{1}, "class_nameTab");
        int multiplex_class_index = (case_expr->get_multiplex())->get_type();
        riscv_emit::emit_load_word(out, TempRegister{1}, MemoryLocation{4 * multiplex_class_index, TempRegister{1}});
        riscv_emit::emit_store_word(out, TempRegister{1}, MemoryLocation{0, StackPointer{}});
        print_decr_stack_pointer_4(out);
        riscv_emit::emit_jump_and_link(out, "_case_abort_no_match");
        only_incr_stack_pointer_pos_n(16);
        riscv_emit::emit_label(out, case_end_label);
        //clean up the multiplex from the stack
        riscv_emit::emit_comment(out, "Case expression cleanup: deallocating multiplex from the stack");
        print_incr_stack_pointer_4(out);
    }
    return ;//to be implemented
}

void CoolCodegen::generate_method_in(ostream &out){
    stack_pointer_pos = 0;
    riscv_emit::emit_add(out, FramePointer{}, StackPointer{}, ZeroRegister{});
    riscv_emit::emit_store_word(out, ReturnAddress{}, MemoryLocation{0, StackPointer{}});
    //riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);
    print_decr_stack_pointer_4(out);
    riscv_emit::emit_store_word(out, ArgumentRegister{0}, MemoryLocation{0, StackPointer{}});
    //riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);
    print_decr_stack_pointer_4(out);
}

void CoolCodegen::generate_method_out(ostream &out){
    riscv_emit::emit_ident(out);
    riscv_emit::emit_comment(out, "Method cleanup and return");
    riscv_emit::emit_load_word(out, ReturnAddress{}, MemoryLocation{0, FramePointer{}});
    //riscv_emit::emit_load_word(out, FramePointer{}, MemoryLocation{-4, FramePointer{}});
    //riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, 12);
    print_incr_stack_pointer_12(out);
    riscv_emit::emit_load_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
    riscv_emit::emit_ident(out);
    out << "ret\n";
    cerr << "stack_pointer_pos at method out: " << stack_pointer_pos << "\n";
    assert(stack_pointer_pos == 4);//because the old fp is still on the stack
}
    
void CoolCodegen::generate_method_call(ostream &out, int idx, int defining_class_index, const string &defining_class_name,const string &method_name){
    // TO BE IMPLEMENTED
    //riscv_emit::emit_comment(out, "Method call generation to be implemented for " + class_name + "." + method_name);
    string full_method_name = defining_class_name + "." + method_name;
    riscv_emit::emit_globl(out, full_method_name);
    riscv_emit::emit_label(out, full_method_name);
    unordered_map<string, mem_place> &curr_attr_locations = class_attr_locations[idx];
    for (auto it = curr_attr_locations.begin(); it != curr_attr_locations.end(); ++it){
        cerr << "Pushing attribute " << it->first << " at offset " << it->second.offset 
             << " isattr: " << it->second.isattr << " for method " << full_method_name << "\n";
        local_var_locations[it->first].push(it->second);
    }
    cerr << "Generating method " << full_method_name << "\n";
    generate_method_in(out);
    vector<string> args_names = class_table_->get_argument_names(defining_class_index, method_name);
    int num_args = args_names.size();
    for (int i = 0; i < num_args; ++i){
        /*cerr << "Pushing argument " << args_names[i] << " at offset " << (4 * (num_args + 1 - i)) 
             << " for method " << full_method_name << "\n";
        //0(fp)->ra, 4(fp)->argN, ..., 4 + 4N(fp)->arg1, old fp*/
        local_var_locations[args_names[i]].push(mem_place((num_args - i) * 4, false));//num_args + 1 - i
    }
    const Expr* method_body = class_table_->get_method_body(defining_class_index, method_name);
    //negative_offset_fp = -8;//0(fp)->ra, 4(fp)->old fp, -4(fp)->self object pointer
    generate_expr_code(out, method_body);
    /*maybe the only place not swapped with print_incr/print_decr with the because
    stack_pointer_pos has to be update during the "execution" of the caller
    Also the arguments are above the fp*/
    riscv_emit::emit_comment(out, "Cleaning up argument locations for method " + full_method_name);
    riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, 4 * num_args);
    generate_method_out(out);
    for (int i = 0; i < num_args; ++i){
        assert(local_var_locations.find(args_names[i]) != local_var_locations.end());
        assert(!local_var_locations[args_names[i]].empty());
        assert(local_var_locations[args_names[i]].top().offset == (int)((num_args - i) * 4));
        local_var_locations[args_names[i]].pop();
        assert(local_var_locations[args_names[i]].empty());
    }

    for (auto it = curr_attr_locations.begin(); it != curr_attr_locations.end(); ++it){
        assert(local_var_locations.find(it->first) != local_var_locations.end());
        assert(!local_var_locations[it->first].empty());
        assert(local_var_locations[it->first].top() == it->second);
        local_var_locations[it->first].pop();
        assert(local_var_locations[it->first].empty());
    }
}

void CoolCodegen::generate(ostream &out) {
    class_table_->normalize_indexes();
    class_table_->compute_sub_hierarchy_sizes();
    // Emit code here.

    /*string fout_name =  file_name_ + ".txt";
    ofstream fout(fout_name);
    fout << "# Generating code for file: " << file_name_ << "\n";
    const vector <string> &class_names = class_table_->get_class_names();
    for (int i = 0; i < class_table_->get_num_of_classes(); ++i){
        string class_name = class_names[i];
        fout << "# Class " << class_name << " at index " << i 
        << " with parent index: " << class_table_->get_parent_index(i) << "\n";
    }
    fout << "# Code generattion for file " << file_name_ << " completed.\n";
    fout << "\n";
    fout.close();
    */

    // 1. create an "object model class table" that uses the class_table_ to compute the layout of objects in memory
    int num_classes = class_table_->get_num_of_classes();

    built_classes.resize(num_classes, false);
    class_attr_locations.resize(num_classes);
    dispatch_table_labels.resize(num_classes);
    class_sizes.resize(num_classes, 0);
    for (int i = 0; i < num_classes; ++i)
        if (!built_classes[i]) build_class(i);
    // 2. create a class to contain static constants (to be emitted at the end)
    
    code_consts.add_bool(false);
    code_consts.add_bool(true);
    code_consts.add_int(0);//empty string len
    code_consts.add_string("");//empty string
    code_consts.add_string(file_name_);//source file name
    code_consts.add_int(file_name_.size());
    for (int i = 0; i < num_classes; ++i){
        string name(class_table_->get_name(i));
        string label = name + "_className";
        //code_consts.add_string(name);
        code_consts.string_consts[name] = label;
        code_consts.add_int(emitted_string_length(name));
    }

    riscv_emit::emit_text_segment_tag(out);
    //generate code for classes methods
    for (int i = 0; i < num_classes; ++i){
        in_class = i;
        string class_name = string(class_table_->get_name(i));
        vector<pair<string, int>> all_methods = class_table_->get_all_methods(i);
        int num_methods = all_methods.size();
        for (int j = 0; j < num_methods; ++j){
            string method_name = all_methods[j].first;
            int defining_class_index = all_methods[j].second;
            assert(defining_class_index != -1);
            if (defining_class_index != i) continue; //method implemented in a parent class
            string defining_class_name = string(class_table_->get_name(defining_class_index));
            if (is_basic_class(defining_class_name)) continue; //implementation by the runtime
            generate_method_call(out, i, defining_class_index, defining_class_name, method_name);
        }
    }

    Object_init(out, "Object_init");
    Object_init(out, "IO_init");
    Object_init(out, "Int_init");
    Object_init(out, "Bool_init");
    String_init(out);
    //init user defined classes
    for (int i = 0; i < num_classes; ++i){
        string class_name = string(class_table_->get_name(i));
        if (is_basic_class(class_name)) continue;
        vector<string> all_attr_names = class_table_->get_all_attributes(i);
        riscv_emit::emit_globl(out, class_name + "_init");
        riscv_emit::emit_label(out, class_name + "_init");
        cerr << "Generating init for class " << class_name << "\n";
        generate_method_in(out);
        int parent_idx = class_table_->get_parent_index(i);
        riscv_emit::emit_store_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
        //riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);
        print_decr_stack_pointer_4(out);
        riscv_emit::emit_call(out, string(class_table_->get_name(parent_idx)) + "_init");
        only_incr_stack_pointer_pos_n(4);

        unordered_map<string, mem_place> &curr_attr_locations = class_attr_locations[i];
        for (auto it = curr_attr_locations.begin(); it != curr_attr_locations.end(); ++it){
            /*cerr << "Pushing attribute " << it->first << " at offset " << it->second.offset 
                << " isattr: " << it->second.isattr << " for method " << full_method_name << "\n";*/
            local_var_locations[it->first].push(it->second);
        }
        for (const string &attr_name : all_attr_names){
            const Expr* initializer = class_table_->transitive_get_attribute_initializer(class_name, attr_name);
            if (initializer != nullptr){
                generate_expr_code(out, initializer);
                // result in a0
                // store a0 to attribute location
                mem_place &attr_location = class_attr_locations[i][attr_name];
                /* self is in fp - 4
                t1 = -4(fp)
                a0 -> offset(t1)
                */
                riscv_emit::emit_load_word(out, TempRegister{1}, MemoryLocation{-4, FramePointer{}});
                riscv_emit::emit_store_word(out, ArgumentRegister{0}, MemoryLocation{attr_location.offset, TempRegister{1}});
            }
        }
        riscv_emit::emit_load_word(out, ArgumentRegister{0}, MemoryLocation{-4, FramePointer{}});
        generate_method_out(out);
        for (auto it = curr_attr_locations.begin(); it != curr_attr_locations.end(); ++it){
            assert(local_var_locations.find(it->first) != local_var_locations.end());
            assert(!local_var_locations[it->first].empty());
            assert(local_var_locations[it->first].top() == it->second);
            local_var_locations[it->first].pop();
            assert(local_var_locations[it->first].empty());
        }
    }

    out << "\n";
    riscv_emit::emit_data_segment_tag(out);
    riscv_emit::emit_p2align(out, 2);
    riscv_emit::emit_globl(out, "class_nameTab");
    riscv_emit::emit_label(out, "class_nameTab");
    for (int i = 0; i < num_classes; ++i){
        string class_name_label = string(class_table_->get_name(i)) + "_className";
        riscv_emit::emit_word(out, class_name_label);
    }

    for (auto it = code_consts.bool_consts.begin(); it != code_consts.bool_consts.end(); ++it){
        int class_tag = class_table_->get_index("Bool");
        assert(class_tag != -1);
        int value = it->first;
        string label = it->second;
        out << "\n";
        riscv_emit::emit_word(out, -1, "GC tag");
        riscv_emit::emit_label(out, label);
        //riscv_emit::emit_word(out, b ? 1 : 0, "boolean value");
        riscv_emit::emit_word(out, class_tag, "class tag");
        riscv_emit::emit_word(out, 4, "object size; 4 words (16 bytes); GC tag not included");
        //riscv_emit::emit_word(out, "Bool_dispTab");
        riscv_emit::emit_word(out, dispatch_table_labels[class_tag]);
        riscv_emit::emit_word(out, value ? 1 : 0, "boolean value");
    }

    for (auto it = code_consts.int_consts.begin(); it != code_consts.int_consts.end(); ++it){
        int class_tag = class_table_->get_index("Int");
        assert(class_tag != -1);
        int value = it->first;
        string label = it->second;
        out << "\n";
        riscv_emit::emit_word(out, -1, "GC tag");
        riscv_emit::emit_label(out, label);
        riscv_emit::emit_word(out, class_tag, "class tag");
        riscv_emit::emit_word(out, 4, "object size; 4 words (16 bytes); GC tag not included");
        //riscv_emit::emit_word(out, "Int_dispTab");
        riscv_emit::emit_word(out, dispatch_table_labels[class_tag]);
        riscv_emit::emit_word(out, value, "int value");
    }

    for (auto it = code_consts.string_consts.begin(); it != code_consts.string_consts.end(); ++it){
        static int class_tag = class_table_->get_index("String");
        assert(class_tag != -1);
        string value = it->first;
        string label = it->second;
        int value_length = emitted_string_length(value);
        int sz = (value_length + 1) / 4 + ((value_length + 1) % 4 ==0 ? 0 : 1) + 4;//str, length, class_tag, obj_size, disp_table
        out << "\n";
        riscv_emit::emit_word(out, -1, "GC tag");
        riscv_emit::emit_label(out, label);
        riscv_emit::emit_word(out, class_tag, "class tag");
        riscv_emit::emit_word(out, sz, "object size; ");
        //riscv_emit::emit_word(out, "String_dispTab");
        riscv_emit::emit_word(out, dispatch_table_labels[class_tag]);
        assert(code_consts.int_consts.find(value_length) != code_consts.int_consts.end());
        string length_label = code_consts.int_consts[value_length];
        riscv_emit::emit_word(out, length_label);
        riscv_emit::emit_string(out, "\"" + value + "\"", "string content");
        //riscv_emit::emit_byte(out, 0, "null terminator");
        riscv_emit::emit_p2align(out, 2);
    }
    // 3. emit code for method bodies; possibly append to static constants
    //risc5
    // 4. emit prototype objects
    for (int i = 0; i < num_classes; ++i){
        out << "\n";
        riscv_emit::emit_word(out, -1, "GC tag");
        string class_name = string(class_table_->get_name(i));
        riscv_emit::emit_globl(out, class_name + "_protObj");
        riscv_emit::emit_label(out, class_name + "_protObj");
        riscv_emit::emit_word(out, i, "class tag");
        
        // initialize attribute to default value
        // all attributes to 0 / null pointer
        riscv_emit::emit_word(out, class_sizes[i], "object size;");
        riscv_emit::emit_word(out, dispatch_table_labels[i]);
        if (class_name == "Int")
            riscv_emit::emit_word(out, 0, "int value default");
        else if (class_name == "String"){
            riscv_emit::emit_word(out, code_consts.int_consts[0]);
            riscv_emit::emit_word(out, 0, "string content default");
        }
        else if (class_name == "Bool")
            riscv_emit::emit_word(out, 0, "bool value default");
        else{
            int num_attrs = class_attr_locations[i].size();
            for (int j = 0; j < num_attrs; ++j){
                string attr_name = class_table_->get_all_attributes(i)[j];
                int attr_type_index = class_table_->transitive_get_attribute_type(i, attr_name).value();
                assert((j+3)*4 == class_attr_locations[i][attr_name].offset);
                string type_name = string(class_table_->get_name(attr_type_index));
                if (type_name == "Int")
                    riscv_emit::emit_word(out, code_consts.int_consts[0]);
                else if (type_name == "String")
                    riscv_emit::emit_word(out, code_consts.string_consts[""]);
                else if (type_name == "Bool")
                    riscv_emit::emit_word(out, code_consts.bool_consts[false]);
                else riscv_emit::emit_word(out, 0, "attribute " + attr_name + " default value");
            }
        }
    }

    // 5. emit dispatch tables
    riscv_emit::emit_comment(out, "------------- Dispatch tables ------------------------------------------------");

    for (int i = 0; i < num_classes; ++i){
        string class_name = string(class_table_->get_name(i));
        string dispatch_label = dispatch_table_labels[i];
        riscv_emit::emit_globl(out, dispatch_label);
        riscv_emit::emit_label(out, dispatch_label);
        vector<pair<string, int>> all_methods = class_table_->get_all_methods(i);
        for (auto &method_pair : all_methods){
            string method_name = method_pair.first;
            int defining_class_index = method_pair.second;
            string defining_class_name = string(class_table_->get_name(defining_class_index));
            riscv_emit::emit_word(out, defining_class_name + "." + method_name);
        }
    }

    riscv_emit::emit_globl(out, "_int_tag");
    riscv_emit::emit_label(out, "_int_tag");
    riscv_emit::emit_word(out, class_table_->get_index("Int"));
    riscv_emit::emit_globl(out, "_bool_tag");
    riscv_emit::emit_label(out, "_bool_tag");
    riscv_emit::emit_word(out, class_table_->get_index("Bool"));
    riscv_emit::emit_globl(out, "_string_tag");
    riscv_emit::emit_label(out, "_string_tag");
    riscv_emit::emit_word(out, class_table_->get_index("String"));
    // 6. emit initialization methods for classes

    const vector<string> & sth = class_table_->get_class_names();
    for (int i = 0; i < num_classes; ++i){
        cerr << "Class " << sth[i] << " has idx " << i <<
        " and sub hier. size " << class_table_->get_sub_hierarchy_size(i) << "\n";
    }

    // 7. emit class name table Done

    // 8. emit static constants Done

    // Extra tip: implement code generation for expressions in a separate class and reuse it for method impls and init methods.


    // The following is an example manually written assembly that prints out "hello, world".
    
    // Start your work by removing it.
    /*   out << "# 001.hello_world.example.s is inlined here\n\
.text\n\
\n\
_inf_loop:\n\
    j _inf_loop\n\
\n\
.globl Main.main\n\
Main.main:\n\
    # stack discipline:\n\
    # caller:\n\
    # - self object is passed in a0\n\
    # - control link is pushed first on the stack\n\
    # - arguments are pushed in reverse order on the stack\n\
    # callee:\n\
    # - activation frame starts at the stack pointer\n\
    add fp, sp, 0\n\
    # - previous return address is first on the activation frame\n\
    sw ra, 0(sp)\n\
    addi sp, sp, -4\n\
    # - size of activation frame is fixed per method, because it depends on\n\
    #   number of arguments\n\
    # - fp points to sp\n\
    # - sp points to next free stack memory\n\
    # before using saved registers (s1 -- s11), push them on the stack\n\
\n\
    # stack discipline:\n\
    # caller:\n\
    # - self object is passed in a0\n\
    # self already is a0, so no-op\n\
    # - control link is pushed first on the stack\n\
    sw fp, 0(sp)\n\
    addi sp, sp, -4\n\
    # - arguments are pushed in reverse order on the stack\n\
    la t0, _string1.content\n\
    sw t0, 0(sp)\n\
    addi sp, sp, -4\n\
\n\
    jal IO.out_string\n\
\n\
    # stack discipline:\n\
    # callee:\n\
    # - restore used saved registers (s1 -- s11) from the stack\n\
    # - ra is restored from first word on activation frame\n\
    lw ra, 0(fp)\n\
    # - ra, arguments, and control link are popped from the stack\n\
    addi sp, sp, 8\n\
    # - fp is restored from control link\n\
    lw fp, 0(sp)\n\
    # - result is stored in a0\n\
    # caller:\n\
    # - read return value from a0\n\
    ret\n\
\n\
.data\n\
# ------------- Name table of classes ------------------------------------------\n\
.p2align 2\n\
.globl class_nameTab\n\
class_nameTab:\n\
    .word Object_className\n\
    .word IO_className\n\
    .word Int_className\n\
    .word Bool_className\n\
    .word String_className\n\
    .word Main_className\n\
\n\
    .word -1 # GC tag\n\
Object_classNameLength:\n\
    .word 2  # class tag;       2 for Int\n\
    .word 4  # object size;     4 words (16 bytes); GC tag not included\n\
    .word 0  # dispatch table;  Int has no methods\n\
    .word 6  # first attribute; value of Int; default is 0\n\
\n\
    .word -1 # GC tag\n\
Object_className:\n\
    .word 4  # class tag;       4 for String\n\
    .word 6  # object size;     6 words (24 bytes); GC tag not included\n\
    .word String_dispTab\n\
    .word Object_classNameLength  # first attribute; pointer length\n\
    .string \"Object\"\n\
    .byte 0\n\
\n\
    .word -1 # GC tag\n\
IO_classNameLength:\n\
    .word 2  # class tag;       2 for Int\n\
    .word 4  # object size;     4 words (16 bytes); GC tag not included\n\
    .word 0  # dispatch table;  Int has no methods\n\
    .word 2  # first attribute; value of Int; default is 0\n\
\n\
    .word -1 # GC tag\n\
IO_className:\n\
    .word 4  # class tag;       4 for String\n\
    .word 5  # object size;     5 words (20 bytes); GC tag not included\n\
    .word String_dispTab\n\
    .word IO_classNameLength  # first attribute; pointer length\n\
    .string \"IO\" # includes terminating null char\n\
    .byte 0\n\
\n\
    .word -1 # GC tag\n\
Int_classNameLength:\n\
    .word 2  # class tag;       2 for Int\n\
    .word 4  # object size;     4 words (16 bytes); GC tag not included\n\
    .word 0  # dispatch table;  Int has no methods\n\
    .word 3  # first attribute; value of Int; default is 0\n\
\n\
    .word -1 # GC tag\n\
Int_className:\n\
    .word 4  # class tag;       4 for String\n\
    .word 5  # object size;     5 words (20 bytes); GC tag not included\n\
    .word String_dispTab\n\
    .word Int_classNameLength  # first attribute; pointer length\n\
    .string \"Int\" # includes terminating null char\n\
\n\
    .word -1 # GC tag\n\
Bool_classNameLength:\n\
    .word 2  # class tag;       2 for Int\n\
    .word 4  # object size;     4 words (16 bytes); GC tag not included\n\
    .word 0  # dispatch table;  Int has no methods\n\
    .word 4  # first attribute; value of Int; default is 0\n\
\n\
    .word -1 # GC tag\n\
Bool_className:\n\
    .word 4  # class tag;       4 for String\n\
    .word 6  # object size;     6 words (24 bytes); GC tag not included\n\
    .word String_dispTab\n\
    .word Bool_classNameLength  # first attribute; pointer length\n\
    .string \"Bool\" # includes terminating null char\n\
    .byte 0\n\
    .byte 0\n\
    .byte 0\n\
\n\
    .word -1 # GC tag\n\
String_classNameLength:\n\
    .word 2  # class tag;       2 for Int\n\
    .word 4  # object size;     4 words (16 bytes); GC tag not included\n\
    .word 0  # dispatch table;  Int has no methods\n\
    .word 6  # first attribute; value of Int; default is 0\n\
\n\
    .word -1 # GC tag\n\
String_className:\n\
    .word 4  # class tag;       4 for String\n\
    .word 6  # object size;     6 words (24 bytes); GC tag not included\n\
    .word String_dispTab\n\
    .word String_classNameLength  # first attribute; pointer length\n\
    .string \"String\" # includes terminating null char\n\
    .byte 0\n\
\n\
    .word -1 # GC tag\n\
Main_classNameLength:\n\
    .word 2  # class tag;       2 for Int\n\
    .word 4  # object size;     4 words (16 bytes); GC tag not included\n\
    .word 0  # dispatch table;  Int has no methods\n\
    .word 4  # first attribute; value of Int; default is 0\n\
\n\
    .word -1 # GC tag\n\
Main_className:\n\
    .word 4  # class tag;       4 for String\n\
    .word 6  # object size;     6 words (24 bytes); GC tag not included\n\
    .word String_dispTab\n\
    .word Main_classNameLength  # first attribute; pointer length\n\
    .string \"Main\" # includes terminating null char\n\
    .byte 0\n\
    .byte 0\n\
    .byte 0\n\
\n\
# ------------- Prototype objects ----------------------------------------------\n\
    .p2align 2\n\
    .word -1 # GC tag\n\
.globl Object_protObj\n\
Object_protObj:\n\
    .word 0  # class tag;       0 for Object\n\
    .word 3  # object size;     3 words (12 bytes); GC tag not included\n\
    .word Object_dispTab\n\
\n\
    .word -1 # GC tag\n\
.globl IO_protObj\n\
IO_protObj:\n\
    .word 1  # class tag;       1 for IO\n\
    .word 3  # object size;     3 words (12 bytes); GC tag not included\n\
    .word IO_dispTab\n\
\n\
    .word -1 # GC tag\n\
.globl Int_protObj\n\
Int_protObj:\n\
    .word 2  # class tag;       2 for Int\n\
    .word 4  # object size;     4 words (16 bytes); GC tag not included\n\
    .word 0  # dispatch table;  Int has no methods\n\
    .word 0  # first attribute; value of Int; default is 0\n\
\n\
    .word -1 # GC tag\n\
.globl Bool_protObj\n\
Bool_protObj:\n\
    .word 3  # class tag;       3 for Bool\n\
    .word 4  # object size;     4 words (16 bytes); GC tag not included\n\
    .word 0  # dispatch table;  Bool has no methods\n\
    .word 0  # first attribute; value of Bool; default is 0; means false\n\
\n\
    .word -1 # GC tag\n\
.globl String_protObj\n\
String_protObj:\n\
    .word 4  # class tag;       4 for String\n\
    .word 5  # object size;     5 words (20 bytes); GC tag not included\n\
    .word String_dispTab\n\
    .word 0  # first attribute; pointer to Int that is the length of the String\n\
    .word 0  # second attribute; terminating 0 character, since \"\" is default\n\
\n\
    .word -1 # GC tag\n\
.globl Main_protObj\n\
Main_protObj:\n\
    .word 5  # class tag;       5 for Main\n\
    .word 3  # object size;     3 words (12 bytes); GC tag not included\n\
    .word Main_dispTab\n\
\n\
# ------------- Dispatch tables ------------------------------------------------\n\
.globl Object_dispTab\n\
Object_dispTab:\n\
    .word Object.abort\n\
    .word Object.type_name\n\
    .word Object.copy\n\
\n\
.globl IO_dispTab\n\
IO_dispTab:\n\
    .word Object.abort\n\
    .word Object.type_name\n\
    .word Object.copy\n\
    .word IO.out_string\n\
    .word IO.in_string\n\
    .word IO.out_int\n\
    .word IO.in_int\n\
\n\
.globl Int_dispTab\n\
Int_dispTab:\n\
    .word Object.abort\n\
    .word Object.type_name\n\
    .word Object.copy\n\
\n\
.globl Bool_dispTab\n\
Bool_dispTab:\n\
    .word Object.abort\n\
    .word Object.type_name\n\
    .word Object.copy\n\
\n\
.globl String_dispTab\n\
String_dispTab:\n\
    .word Object.abort\n\
    .word Object.type_name\n\
    .word Object.copy\n\
    .word String.length\n\
    .word String.concat\n\
    .word String.substr\n\
\n\
# no need to export symbols for user-defined types\n\
Main_dispTab:\n\
    .word Object.abort\n\
    .word Object.type_name\n\
    .word Object.copy\n\
    .word IO.out_string\n\
    .word IO.out_int\n\
    .word IO.in_string\n\
    .word IO.in_int\n\
    .word Main.main\n\
\n\
# ----------------- Init methods -----------------------------------------------\n\
\n\
.globl Object_init\n\
Object_init:\n\
    # Most of the `init` functions of the default types are no-ops, so the\n\
    # implementation is the same.\n\
\n\
    # stack discipline:\n\
    # callee:\n\
    # - activation frame starts at the stack pointer\n\
    add fp, sp, 0\n\
    # - previous return address is first on the activation frame\n\
    sw ra, 0(sp)\n\
    addi sp, sp, -4\n\
    # before using saved registers (s1 -- s11), push them on the stack\n\
\n\
    # no op\n\
\n\
    # stack discipline:\n\
    # callee:\n\
    # - restore used saved registers (s1 -- s11) from the stack\n\
    # - ra is restored from first word on activation frame\n\
    lw ra, 0(fp)\n\
    # - ra, arguments, and control link are popped from the stack\n\
    addi sp, sp, 8\n\
    # - fp is restored from control link\n\
    lw fp, 0(sp)\n\
    # - result is stored in a0\n\
\n\
    ret\n\
\n\
\n\
.globl IO_init\n\
IO_init:\n\
    # Most of the `init` functions of the default types are no-ops, so the\n\
    # implementation is the same.\n\
\n\
    add fp, sp, 0\n\
    sw ra, 0(sp)\n\
    addi sp, sp, -4\n\
\n\
    # no op\n\
\n\
    lw ra, 0(fp)\n\
    addi sp, sp, 8\n\
    lw fp, 0(sp)\n\
    ret\n\
\n\
\n\
# Initializes an object of class Int passed in a0. In practice, a no-op, since\n\
# Int_protObj already has the first (and only) attribute set to 0.\n\
.globl Int_init\n\
Int_init:\n\
    # Most of the `init` functions of the default types are no-ops, so the\n\
    # implementation is the same.\n\
\n\
    add fp, sp, 0\n\
    sw ra, 0(sp)\n\
    addi sp, sp, -4\n\
\n\
    # no op\n\
\n\
    lw ra, 0(fp)\n\
    addi sp, sp, 8\n\
    lw fp, 0(sp)\n\
    ret\n\
\n\
\n\
# Initializes an object of class Bool passed in a0. In practice, a no-op, since\n\
# Bool_protObj already has the first (and only) attribute set to 0.\n\
.globl Bool_init\n\
Bool_init:\n\
    # Most of the `init` functions of the default types are no-ops, so the\n\
    # implementation is the same.\n\
\n\
    add fp, sp, 0\n\
    sw ra, 0(sp)\n\
    addi sp, sp, -4\n\
\n\
    # no op\n\
\n\
    lw ra, 0(fp)\n\
    addi sp, sp, 8\n\
    lw fp, 0(sp)\n\
    ret\n\
\n\
\n\
# Initializes an object of class String passed in a0. Allocates a new Int to\n\
# store the length of the String and links the length pointer to it. Returns the\n\
# initialized String in a0.\n\
#\n\
# Used in `new String`, but useless, in general, since it creates an empty\n\
# string. String only has methods `length`, `concat`, and `substr`.\n\
.globl String_init\n\
String_init:\n\
    # In addition to the default behavior, copies the Int prototype object and\n\
    # uses that as the length, rather than the prototype object directly. No\n\
    # practical reason for this, other than simulating the default init logic for\n\
    # an object with attributes.\n\
\n\
    add fp, sp, 0\n\
    sw ra, 0(sp)\n\
    addi sp, sp, -4\n\
\n\
    # store String argument\n\
    sw s1, 0(sp)\n\
    addi sp, sp, -4\n\
    add s1, a0, zero\n\
\n\
    # copy Int prototype first\n\
\n\
    la a0, Int_protObj\n\
    sw fp, 0(sp)\n\
    addi sp, sp, -4\n\
\n\
    call Object.copy\n\
\n\
    sw a0, 12(s1)      # store new Int as length; value of Int is 0 by default\n\
\n\
    add a0, s1, zero   # restore String argument\n\
\n\
    addi sp, sp, 4\n\
    lw s1, 0(sp)\n\
    lw ra, 0(fp)\n\
    addi sp, sp, 8\n\
    lw fp, 0(sp)\n\
\n\
    ret\n\
\n\
\n\
.globl Main_init\n\
Main_init:\n\
    # stack discipline:\n\
    # callee:\n\
    # - activation frame starts at the stack pointer\n\
    add fp, sp, 0\n\
    # - previous return address is first on the activation frame\n\
    sw ra, 0(sp)\n\
    addi sp, sp, -4\n\
    # before using saved registers (s1 -- s11), push them on the stack\n\
\n\
    # no op\n\
\n\
    # stack discipline:\n\
    # callee:\n\
    # - restore used saved registers (s1 -- s11) from the stack\n\
    # - ra is restored from first word on activation frame\n\
    lw ra, 0(fp)\n\
    # - ra, arguments, and control link are popped from the stack\n\
    addi sp, sp, 8\n\
    # - fp is restored from control link\n\
    lw fp, 0(sp)\n\
    # - result is stored in a0\n\
\n\
    ret\n\
\n\
\n\
# ------------- Class object table ---------------------------------------------\n\
class_objTab:\n\
    .word Object_protObj\n\
    .word Object_init\n\
    .word IO_protObj\n\
    .word IO_init\n\
    .word Int_protObj\n\
    .word Int_init\n\
    .word Bool_protObj\n\
    .word Bool_init\n\
    .word String_protObj\n\
    .word String_init\n\
    .word Main_protObj\n\
    .word Main_init\n\
\n\
    .word -1 # GC tag\n\
_string1.length:\n\
    .word 2  # class tag;       2 for Int\n\
    .word 4  # object size;     4 words (16 bytes); GC tag not included\n\
    .word 0  # dispatch table;  Int has no methods\n\
    .word 13  # first attribute; value of Int\n\
\n\
    .word -1 # GC tag\n\
_string1.content:\n\
    .word 4  # class tag;       4 for String\n\
    .word 8  # object size;     8 words (16 + 16 bytes); GC tag not included\n\
    .word String_dispTab\n\
    .word _string1.length # first attribute; pointer length\n\
    .string \"hello world!\\n\" # includes terminating null char\n\
    .byte 0\n\
    .byte 0\n\
.globl _bool_tag\n\
_bool_tag:\n\
    .word 3\n\
.globl _int_tag\n\
_int_tag:\n\
    .word 2\n\
.globl _string_tag\n\
_string_tag:\n\
    .word 4\n\
";*/
    
}
