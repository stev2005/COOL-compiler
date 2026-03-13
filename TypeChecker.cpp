#include "TypeChecker.h"

#include <string>
#include <vector>
#include <cassert>

#include "CoolParser.h"

using namespace std;

vector<string> TypeChecker::check(CoolParser *parser) {
    visitProgram(parser->program());
    parser->reset();
    return std::move(errors);
}

void TypeChecker::check_class_methods(CoolParser::ClassContext *ctx) {
    string class_name = ctx->TYPEID(0)->getText();
    auto &methods = class_methods[current_class_index];
    vector<string> method_names = class_methods[current_class_index].get_names();
    unordered_set<string> method_visited;
    auto methods_def_decl = ctx->method();
    int method_pos = 0;
    for (int m_idx = 0; m_idx < methods_def_decl.size() && method_pos < method_names.size(); ++m_idx) {
        auto current_method = methods_def_decl[m_idx];
        string name = current_method->OBJECTID()->getText();
        if (method_visited.find(name) != method_visited.end()) continue;
        if (!methods.contains(name)) continue; // it means that this method had problem in declaration phase
        auto method_signature = *methods.get_signature(name);
        auto argument_names = methods.get_argument_names(name);
        auto formals = current_method->formal();
        if (formals.size() != method_signature.size() - 1) continue;
        bool dif_types = false;
        for (int i = 0; i < formals.size(); ++i) {
            string formal_name = formals[i]->OBJECTID()->getText();
            int formal_type = type_name_to_index[formals[i]->TYPEID()->getText()];
            if (argument_names[i] != formal_name ||
                method_signature[i] != formal_type) {
                dif_types = true;
                break;
            }
        }
        if (dif_types) continue;
        // now we have found the method definition corresponding to method_names[method_pos]
        method_visited.insert(name);
        // push variables into var_stack
        for (int i = 0; i < formals.size(); ++i)
            var_stack[argument_names[i]].push(method_signature[i]);
        // typecheck the method body
        int get_type = any_cast<int>(visitExpr(current_method->expr()));
        int ret_type = method_signature.back();
        if (get_type == NO_TYPE_INT) continue;// skip further checks if type is already an error
        if (ret_type == SELF_TYPE_INT && get_type != SELF_TYPE_INT){
            errors.push_back("In class `" + class_name + "` method `" + name + "`: `" + index_to_type_name[get_type] + "` is not `SELF_TYPE`"
                + ": type of method body is not a subtype of return type");
            continue;
        }
        string str_get_type = index_to_type_name[get_type];
        string str_ret_type = index_to_type_name[ret_type];
        if (get_type == SELF_TYPE_INT) get_type = current_class_index;
        if (ret_type == SELF_TYPE_INT) ret_type = current_class_index;
        if (get_type != ret_type && lca_finder.lca(ret_type, get_type) != ret_type) //get_type is not a subtype of ret_type
            errors.push_back("In class `" + class_name + "` method `" + name + "`: `" + str_get_type + "` is not `" + str_ret_type
                + "`" +  ": type of method body is not a subtype of return type");
        // pop variables from var_stack
        for (int i = 0; i < formals.size(); ++i){
            assert(!var_stack[argument_names[i]].empty()
                   && var_stack[argument_names[i]].top() == method_signature[i]);
            var_stack[argument_names[i]].pop();
        }
        method_pos++;
        if (method_pos >= method_names.size()) break;
    }
}

std::any TypeChecker::visitClass(CoolParser::ClassContext *ctx) {
    string class_name = ctx->TYPEID(0)->getText();
    current_class_index = type_name_to_index.at(class_name);
    int cnt_class_attrs = ctx->attr().size();
    int attr_pos = 0;
    vector<bool> checked_attrs(class_attributes[current_class_index].get_names().size(), false);
    int count_own_attributes = count_class_own_attributes[current_class_index];
    //insert inherited attributes in var_stack
    vector<string> class_attr_names = class_attributes[current_class_index].get_names();
    int count_attributes = class_attr_names.size();
    for (int i = count_attributes - 1; i >= count_own_attributes; --i){
        //inherited attributes are checked in the parent class
        var_stack[class_attr_names[i]].push(*class_attributes[current_class_index].get_type(class_attr_names[i]));
        checked_attrs[i] = true;
    }
    for (int i = 0; i < count_own_attributes; ++i)
        var_stack[class_attr_names[i]].push(*class_attributes[current_class_index].get_type(class_attr_names[i]));
    int idx_attr_antlr = 0;
    for (int i = 0; i < count_own_attributes; ++i){
        string attr_name = class_attr_names[i];
        int attr_type = *class_attributes[current_class_index].get_type(attr_name);
        auto cur_antlr_attr = ctx->attr(idx_attr_antlr);
        string cur_name = cur_antlr_attr->OBJECTID()->getText();
        int cur_type = type_name_to_index[cur_antlr_attr->TYPEID()->getText()];
        while (attr_name != cur_name && cur_type != attr_type){
            idx_attr_antlr++;
            cur_antlr_attr = ctx->attr(idx_attr_antlr);
            cur_name = cur_antlr_attr->OBJECTID()->getText();
            cur_type = type_name_to_index[cur_antlr_attr->TYPEID()->getText()];
        }
        if (!cur_antlr_attr->expr()) continue;
        int get_type = any_cast<int>(visitExpr(cur_antlr_attr->expr()));
        string str_get_type;
        string str_attr_type;
        if (get_type == NO_TYPE_INT) continue;
        str_get_type = index_to_type_name[get_type];
        str_attr_type = index_to_type_name[attr_type];
        if (get_type == SELF_TYPE_INT) get_type = current_class_index;
        if (attr_type == SELF_TYPE_INT) attr_type = current_class_index;
        if (attr_type != lca_finder.lca(attr_type, get_type))
            errors.push_back("In class `" + class_name + "` attribute `" + attr_name + "`: `" + str_get_type + "` is not `" + str_attr_type
                            + "`: type of initialization expression is not a subtype of declared type");
    }

    if (ctx->method().size() != 0) check_class_methods(ctx);

    //for (const auto &attr: class_attributes.at(current_class_index))
    for (int i = 0; i < class_attributes[current_class_index].get_names().size(); ++i) {
        string attr_name = class_attributes[current_class_index].get_names()[i];
        assert(!var_stack[attr_name].empty() && var_stack[attr_name].top() == *class_attributes[current_class_index].get_type(attr_name));
        var_stack[attr_name].pop();
    }
    return any{};

}

std::any TypeChecker::visitExpr(CoolParser::ExprContext *ctx){
    if (ctx->INT_CONST()) return any{type_name_to_index["Int"]};
    if (ctx->STR_CONST()) return any{type_name_to_index["String"]};
    if (ctx->BOOL_CONST()) return any{type_name_to_index["Bool"]};
    if (ctx->OBJECTID(0) && ctx->DOT() && ctx->AT()){//static dispatch
        string method_name = ctx->OBJECTID(0)->getText();
        string static_type_name = ctx->TYPEID(0)->getText();
        bool type_error = false;
        bool continue_formals_check = true;
        int expr_0_type = any_cast<int>(visitExpr(ctx->expr(0)));
        if (type_name_to_index.find(static_type_name) == type_name_to_index.end()){
            errors.push_back("Undefined type `" + static_type_name + "` in static method dispatch");
            type_error = true;
            if (expr_0_type == NO_TYPE_INT) return any{NO_TYPE_INT};
            continue_formals_check = false;
            if (expr_0_type == SELF_TYPE_INT) expr_0_type = current_class_index;
            if (!class_methods[expr_0_type].contains(method_name)){
                errors.push_back("Method `" + method_name + "` not defined for type `" + index_to_type_name[expr_0_type] + "` in static dispatch");
                type_error = true;
                continue_formals_check = false;
            }
            return any{NO_TYPE_INT};
        }

        int static_type = type_name_to_index[static_type_name];
        if (lca_finder.lca(expr_0_type, static_type) != static_type){
            errors.push_back("`"+index_to_type_name[expr_0_type]+"` is not a subtype of `"+index_to_type_name[static_type] + "`");
            type_error = true;
        }
        if (!class_methods[static_type].contains(method_name)){
            errors.push_back("Method `" + method_name + "` not defined for type `" + index_to_type_name[static_type] + "` in static dispatch");
            type_error = true;
            continue_formals_check = false;
        }

        if (!continue_formals_check) return any{NO_TYPE_INT};

        vector<int> signature_types;
        vector<CoolParser::ExprContext *> arguments = ctx->expr(); // expr(0) is the caller
        for (int i = 1; i < arguments.size(); ++i){
            int arg_type = any_cast<int>(visitExpr(arguments[i]));
            if (arg_type == NO_TYPE_INT) return any{NO_TYPE_INT};
            if (arg_type == SELF_TYPE_INT) arg_type = current_class_index;
            signature_types.push_back(arg_type);
        }

        auto method_signature = *class_methods[static_type].get_signature(method_name);
        if (signature_types.size() != method_signature.size() - 1){
            errors.push_back("Method `" + method_name + "` of type `" + index_to_type_name[static_type] + "` called with the wrong number of arguments; "
                             + to_string(method_signature.size() - 1) + " arguments expected, but " + to_string(signature_types.size()) + " provided");
            return any{NO_TYPE_INT};
        }

        for (int i = 0; i < signature_types.size(); ++i){
            int type1 = signature_types[i];
            int type2 = method_signature[i];
            if (lca_finder.lca(type1, type2) != type2){
                errors.push_back("Invalid call to method `" + method_name + "` from class `" + static_type_name+ "`:");
                errors.push_back("  `" +  index_to_type_name[type1] + "` is not a subtype of `" + index_to_type_name[type2] + "`: argument at position " + to_string(i) + " (0-indexed) has the wrong type");
                type_error = true;
            }
        }
        int return_type = method_signature.back();
        return (type_error) ? any{NO_TYPE_INT} : any{return_type};
    }
    else if (ctx->OBJECTID(0) && ctx->DOT()){//method call
        string func_name = ctx->OBJECTID(0)->getText();
        int expr_0_type = any_cast<int>(visitExpr(ctx->expr(0)));
        if (expr_0_type == NO_TYPE_INT) return any{NO_TYPE_INT};
        string expr_0_type_name = index_to_type_name[expr_0_type];
        if (expr_0_type == SELF_TYPE_INT) expr_0_type = current_class_index;
        if (!class_methods[expr_0_type].contains(func_name)){
            errors.push_back("Method `" + func_name + "` not defined for type `" + expr_0_type_name + "` in dynamic dispatch");
            return any{NO_TYPE_INT};
        }
        vector<int> signature_types;
        vector<CoolParser::ExprContext *> arguments = ctx->expr(); // expr(0) is the caller
        for (int i = 1; i < arguments.size(); ++i){
            int arg_type = any_cast<int>(visitExpr(arguments[i]));
            if (arg_type == NO_TYPE_INT) return any{NO_TYPE_INT};
            if (arg_type == SELF_TYPE_INT) arg_type = current_class_index;
            signature_types.push_back(arg_type);
        }
        auto method_signature = *class_methods[expr_0_type].get_signature(func_name);
        int return_type = method_signature.back();
        if (return_type == SELF_TYPE_INT) return_type = expr_0_type;
        if (signature_types.size() != method_signature.size() - 1){
            errors.push_back("In class `" + index_to_type_name[current_class_index] + "` calling method `" + func_name + "`: with wrong number of arguments");
            return any{NO_TYPE_INT};
        }
        bool wrong_args = false;
        for (int i = 0; i < signature_types.size(); ++i){
            int type1 = signature_types[i];
            int type2 = method_signature[i];
            if (lca_finder.lca(type1, type2) != type2){
                wrong_args = true;
                errors.push_back("Invalid call to method `" + func_name + "` from class `" + expr_0_type_name + "`:");
                errors.push_back("  `" +  index_to_type_name[type1] + "` is not a subtype of `" + index_to_type_name[type2] + "`: argument at position " + to_string(i) + " (0-indexed) has the wrong type");
            }
        }
        if (wrong_args) return any{NO_TYPE_INT};
        return any{return_type};
    }

    else if (ctx->OBJECTID(0) && ctx->OPAREN()){//self dynamic dispatch
        string func_name = ctx->OBJECTID(0)->getText();
        if (!class_methods[current_class_index].contains(func_name)){
            errors.push_back("Method `" + func_name + "` not defined for type `" + index_to_type_name[current_class_index] + "` in dynamic dispatch");
            return any{NO_TYPE_INT};
        }

        vector<int> signature_types;
        vector<CoolParser::ExprContext *> arguments = ctx->expr();;
        for (int i = 0; i < arguments.size(); ++i){
            int arg_type = any_cast<int>(visitExpr(arguments[i]));
            if (arg_type == NO_TYPE_INT) return any{NO_TYPE_INT};
            if (arg_type == SELF_TYPE_INT) arg_type = current_class_index;
            signature_types.push_back(arg_type);
        }
        auto method_signature = *class_methods[current_class_index].get_signature(func_name);
        int return_type = method_signature.back();
        if (return_type == SELF_TYPE_INT) return_type = current_class_index;
        if (signature_types.size() != method_signature.size() - 1){
            errors.push_back("In class `" + index_to_type_name[current_class_index] + "` calling method `" + func_name + "`: with wrong number of arguments");
            return any{NO_TYPE_INT};
        }
        for (int i = 0; i < signature_types.size(); ++i){
            int type1 = signature_types[i];
            int type2 = method_signature[i];
            if (lca_finder.lca(type1, type2) != type2){
                errors.push_back("Invalid call to method `" + func_name + "` from class `" + index_to_type_name[current_class_index] + "`:");
                errors.push_back("  `" +  index_to_type_name[type1] + "` is not a subtype of `" + index_to_type_name[type2] + "`: argument at position " + to_string(i) + " (0-indexed) has the wrong type");
                return any{NO_TYPE_INT};
            }
        }
        return any{return_type};
    }

    else if (ctx->OPAREN() && ctx->CPAREN()){//parenthesized expression
        return visitExpr(ctx->expr(0));
    }

    else if (ctx->OCURLY()){//block
        int block_type = NO_TYPE_INT;
        for (auto expr_ctx : ctx->expr())
            block_type = any_cast<int>(visitExpr(expr_ctx));
        return any{block_type};
    }

    else if (ctx->OBJECTID(0) && ctx->expr().size() == 0){///variable
        const string var_name = ctx->OBJECTID(0)->getText();
        if (var_name == "self") return any{SELF_TYPE_INT};
        else{
            if (var_stack.find(var_name) != var_stack.end() && !var_stack[var_name].empty())
                return var_stack[var_name].top();
            else errors.push_back("Variable named `" + var_name + "` not in scope");
        }
        return any{NO_TYPE_INT};
    }

    else if (ctx-> ASSIGN()){
        int sth = -1;
        if (!var_stack[ctx->OBJECTID(0)->getText()].empty())
            sth = var_stack[ctx->OBJECTID(0)->getText()].top();
        string var_name = ctx->OBJECTID(0)->getText();
        int expr_type = any_cast<int>(visitExpr(ctx->expr(0)));
        int var_type;
        if (var_name == "self"){
            errors.push_back("Cannot assign to `self`");
            return any{expr_type};
        }
        if (var_stack[var_name].empty()){
            errors.push_back("Assignee named `" + var_name + "` not in scope");
            return any{expr_type};
        }
        var_type = var_stack[var_name].top();
        string var_type_name;
        if (var_type == SELF_TYPE_INT){
            var_type_name = "SELF_TYPE";
            var_type = current_class_index;
        }
        else var_type_name = index_to_type_name[var_type];
        
        if (expr_type == NO_TYPE_INT) return any{NO_TYPE_INT};
        if (expr_type == SELF_TYPE_INT){
            expr_type = current_class_index;
            if (lca_finder.lca(var_type, expr_type) != var_type)
                errors.push_back("In class `" + index_to_type_name[current_class_index] + "` assignee `" + var_name + "`: `SELF_TYPE` is not `" 
                                 + var_type_name + "`" + ": type of initialization expression is not a subtype of object type");
            return any{SELF_TYPE_INT};
        }
        else if (lca_finder.lca(var_type, expr_type) != var_type){
            errors.push_back("In class `" + index_to_type_name[current_class_index] + "` assignee `" + var_name + "`: `" + index_to_type_name[expr_type]
                            + "` is not `" + var_type_name + "`" + ": type of initialization expression is not a subtype of object type");
            return any{var_type};
        }
        return any{expr_type};            
    }

    else if (ctx->IF()){
        int check_type = any_cast<int>(visitExpr(ctx->expr(0)));
        int then_type = any_cast<int>(visitExpr(ctx->expr(1)));
        int else_type = any_cast<int>(visitExpr(ctx->expr(2)));
        if (check_type == NO_TYPE_INT || then_type == NO_TYPE_INT || else_type == NO_TYPE_INT)
            return any{NO_TYPE_INT};
        if (check_type != type_name_to_index["Bool"]){
            errors.push_back("Type `" + index_to_type_name[check_type] + "` of if-then-else-fi condition is not `Bool`");
            return any{NO_TYPE_INT};
        }
        if (then_type == SELF_TYPE_INT) then_type = current_class_index;
        if (else_type == SELF_TYPE_INT) else_type = current_class_index;
        int lca_type = lca_finder.lca(then_type, else_type);
        return any{lca_type};
    }

    else if (ctx->WHILE()){
        int check_type = any_cast<int>(visitExpr(ctx->expr(0)));
        if (check_type != type_name_to_index["Bool"]){
            errors.push_back("Type `" + index_to_type_name[check_type] + "` of while-loop-pool condition is not `Bool`");
            return any{NO_TYPE_INT};
        }
        int body_type = any_cast<int>(visitExpr(ctx->expr(1)));
        if (check_type == NO_TYPE_INT || body_type == NO_TYPE_INT)
            return any{NO_TYPE_INT};
        return any{type_name_to_index["Object"]};
    }

    else if (ctx->NEW()){
        string type_name = ctx->TYPEID(0)->getText();
        if (type_name_to_index.find(type_name) == type_name_to_index.end()){
            errors.push_back("Attempting to instantiate unknown class `" + type_name + "`");
            return any{NO_TYPE_INT};
        }
        return any{type_name_to_index[type_name]};
    }

    else if (ctx->ISVOID()){
        int expr_type = any_cast<int>(visitExpr(ctx->expr(0)));
        if (expr_type == NO_TYPE_INT) return any{NO_TYPE_INT};
        return any{type_name_to_index["Bool"]};
    }

    else if (ctx->NOT()){
        int expr_type = any_cast<int>(visitExpr(ctx->expr(0)));
        if (expr_type == NO_TYPE_INT) return any{NO_TYPE_INT};
        if (expr_type != type_name_to_index["Bool"])
            errors.push_back("Argument of boolean negation is not of type `Bool`, but of type `" + index_to_type_name[expr_type] + "`");
        return any{type_name_to_index["Bool"]};
    }

    else if (ctx->TILDE()){
        int expr_type = any_cast<int>(visitExpr(ctx->expr(0)));
        if (expr_type == NO_TYPE_INT) return any{NO_TYPE_INT};
        if (expr_type != type_name_to_index["Int"])
            errors.push_back("Argument of integer negation is not of type `Int`, but of type `" + index_to_type_name[expr_type] + "`");
        return any{type_name_to_index["Int"]};
    }

    else if (ctx->STAR() || ctx->SLASH() || ctx->PLUS() || ctx->MINUS()){
        int left_type = any_cast<int>(visitExpr(ctx->expr(0)));
        int right_type = any_cast<int>(visitExpr(ctx->expr(1)));
        bool type_error = false;
        if (left_type != type_name_to_index["Int"]){
            errors.push_back("Left-hand-side of arithmetic expression is not of type `Int`, but of type `" + index_to_type_name[left_type] + "`");
            type_error = true;
        }
        if (right_type != type_name_to_index["Int"]){
            errors.push_back("Right-hand-side of arithmetic expression is not of type `Int`, but of type `" + index_to_type_name[right_type] + "`");
            type_error = true;
        }
        return any{type_name_to_index["Int"]};
    }

    else if (ctx->EQ()){
        int left_type = any_cast<int>(visitExpr(ctx->expr(0)));
        int right_type = any_cast<int>(visitExpr(ctx->expr(1)));
        if (left_type == NO_TYPE_INT || right_type == NO_TYPE_INT)
            return any{type_name_to_index["Bool"]};
        else if (left_type == type_name_to_index["Int"] && right_type != type_name_to_index["Int"])
            errors.push_back("An `Int` can only be compared to another `Int` and not to a `" + index_to_type_name[right_type] + "`");
        else if (left_type == type_name_to_index["String"] && right_type != type_name_to_index["String"])
            errors.push_back("A `String` can only be compared to another `String` and not to a `" + index_to_type_name[right_type] + "`");
        else if (left_type == type_name_to_index["Bool"] && right_type != type_name_to_index["Bool"])
            errors.push_back("A `Bool` can only be compared to another `Bool` and not to a `" + index_to_type_name[right_type] + "`");        
        else if (right_type == type_name_to_index["Int"] && left_type != type_name_to_index["Int"])
            errors.push_back("An `Int` can only be compared to another `Int` and not to a `" + index_to_type_name[left_type] + "`");
        else if (right_type == type_name_to_index["String"] && left_type != type_name_to_index["String"])
            errors.push_back("A `String` can only be compared to another `String` and not to a `" + index_to_type_name[left_type] + "`");
        else if (right_type == type_name_to_index["Bool"] && left_type != type_name_to_index["Bool"])
            errors.push_back("A `Bool` can only be compared to another `Bool` and not to a `" + index_to_type_name[left_type] + "`");
        return any{type_name_to_index["Bool"]};
    }

    else if (ctx->LT() || ctx->LE()){
        //comparison expression
        int left_type = any_cast<int>(visitExpr(ctx->expr(0)));
        int right_type = any_cast<int>(visitExpr(ctx->expr(1)));
        bool type_error = false;
        if (left_type != type_name_to_index["Int"]){
            errors.push_back("Left-hand-side of integer comparison is not of type `Int`, but of type `" + index_to_type_name[left_type] + "`");
            type_error = true;
        }
        if (right_type != type_name_to_index["Int"]){
            errors.push_back("Right-hand-side of integer comparison is not of type `Int`, but of type `" + index_to_type_name[right_type] + "`");
            type_error = true;
        }
        return any{type_name_to_index["Bool"]};
    }

    else if (ctx->LET()){
        // push variables into var_stack
        auto vars = ctx->vardecl();
        int var_count = vars.size();
        vector<bool> to_pop(var_count, false);
        for (size_t i = 0; i < var_count; ++i){
            auto vardecl_ctx = vars[i];
            string var_name = vardecl_ctx->OBJECTID()->getText();
            if (var_name == "self"){
                errors.push_back("Let-binding variable cannot be named `self`");
                continue;
            }
            int type_expr = -1;
            if (vardecl_ctx->expr())type_expr = any_cast<int>(visitExpr(vardecl_ctx->expr()));
            if (type_name_to_index.find(vardecl_ctx->TYPEID()->getText()) == type_name_to_index.end()){
                errors.push_back("Undefined type `" + vardecl_ctx->TYPEID()->getText() + "` in let-binding");
                continue;
            }
            else {
                int var_type = type_name_to_index[vardecl_ctx->TYPEID()->getText()];
                var_stack[var_name].push(var_type);
                to_pop[i] = true;
                if (type_expr != -1){
                    if (type_expr == NO_TYPE_INT) continue;
                    string type_expr_name;
                    string var_type_name;
                    if (var_type == SELF_TYPE_INT && type_expr != SELF_TYPE_INT){
                        errors.push_back("Initializer for variable `" + var_name + "` in let-in expression is of type `" + index_to_type_name[type_expr] +
                            "` which is not a subtype of the declared type `SELF_TYPE`");
                        continue;
                    }
                    if (var_type == SELF_TYPE_INT){
                        var_type_name = "SELF_TYPE";
                        var_type = current_class_index;
                    }
                    else var_type_name = index_to_type_name[var_type];
                    if (type_expr == SELF_TYPE_INT){
                        type_expr_name = "SELF_TYPE";
                        type_expr = current_class_index;
                    }
                    else type_expr_name = index_to_type_name[type_expr];
                    if (lca_finder.lca(var_type, type_expr) != var_type)
                        errors.push_back("Initializer for variable `" + var_name + "` in let-in expression is of type `" + type_expr_name +
                            "` which is not a subtype of the declared type `" + var_type_name + "`");
                }               
            }
        }
        int expr_type = any_cast<int>(visitExpr(ctx->expr(0)));
        // pop variables from var_stack
        for (int i = var_count - 1; i >= 0; --i){
            auto vardecl_ctx = ctx->vardecl(i);
            if (to_pop[i]){
                assert(!var_stack[vardecl_ctx->OBJECTID()->getText()].empty()
                       && var_stack[vardecl_ctx->OBJECTID()->getText()].top() == type_name_to_index[vardecl_ctx->TYPEID()->getText()]);
                var_stack[vardecl_ctx->OBJECTID()->getText()].pop();
            }
        }
        return any{expr_type};
    }

    else if (ctx->CASE()){
        int expr_type = any_cast<int>(visitExpr(ctx->expr(0)));
        if (expr_type == NO_TYPE_INT) return any{NO_TYPE_INT};
        vector<CoolParser::ExprContext *> branches = ctx->expr();
        branches.erase(branches.begin());
        int branches_count = branches.size();
        vector<antlr4::tree::TerminalNode *> branch_var_nodes = ctx->OBJECTID();
        vector<antlr4::tree::TerminalNode *> branch_types_nodes = ctx->TYPEID();
        assert(branches_count == branch_var_nodes.size() && branches_count == branch_types_nodes.size());
        int lca_acumulator = -1;
        set<int> branch_types_in_case;
        set<int> multiple_branch_types;
        for (int i = 0; i < branches_count; ++i){
            string var_name = branch_var_nodes[i]->getText();
            string var_type_name = branch_types_nodes[i]->getText();
            if (type_name_to_index.find(var_type_name) == type_name_to_index.end()){
                errors.push_back("Option `" + var_name + "` in case-of-esac declared to have unknown type `" + var_type_name + "`");
            }
            else{
                int var_type = type_name_to_index[var_type_name];
                if (var_type == SELF_TYPE_INT)
                    errors.push_back("`" + var_name + "` in case-of-esac declared to be of type `SELF_TYPE` which is not allowed");
                else if (branch_types_in_case.find(var_type) == branch_types_in_case.end())
                    branch_types_in_case.insert(var_type);
                else if (multiple_branch_types.find(var_type) == multiple_branch_types.end()){
                    multiple_branch_types.insert(var_type);
                    errors.push_back("Multiple options match on type `" + index_to_type_name[var_type] + "`");
                }
                if (var_name == "self") errors.push_back("Case branch variable cannot be named `self`");
                else var_stack[var_name].push(var_type);
            }
            
            int branch_expr_type = any_cast<int>(visitExpr(branches[i]));
            if (branch_expr_type != NO_TYPE_INT){
                if (lca_acumulator == -1) lca_acumulator = branch_expr_type;
                if (branch_expr_type == SELF_TYPE_INT) 
                    lca_acumulator = lca_finder.lca(lca_acumulator, current_class_index);
                else lca_acumulator = lca_finder.lca(lca_acumulator, branch_expr_type);
            }

            if (var_name != "self" && type_name_to_index.find(var_type_name) != type_name_to_index.end())
                var_stack[var_name].pop();
        }
        return any{(lca_acumulator == -1) ? NO_TYPE_INT : lca_acumulator};
    }

    return any{NO_TYPE_INT};
}