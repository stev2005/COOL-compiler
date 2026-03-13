#include "CoolSemantics.h"
#include <Method.h>
#include <Methods.h>
#include <Attributes.h>

#include <expected>
#include <string>
#include <vector>
#include <unordered_map>
#include <stack>
#include <algorithm>

#include "passes/TypeChecker.h"
#include "../LCA.h"

using namespace std;

const int offset = 1;

unordered_map<string,int> name_to_index;
vector<string> index_to_name;

unordered_map<int, Methods> class_methods;///name_to_index --> int
unordered_map<int, Attributes> class_attributes;
unordered_map<int, int> count_class_own_attributes;



void dfs_detecting_class_loops(vector<vector<int>>& inheritance_graph, int node, int prev, vector<bool>& visited,vector<vector<string>>& inheritance_loops)
{
    visited[node] = true;
    for (auto to : inheritance_graph[node])
    {
        if (to == 0)
            continue;

        if (!visited[to])
            dfs_detecting_class_loops(inheritance_graph, to, node, visited, inheritance_loops);
        else if (to != prev)
        {
            // found a loop
            vector<string> loop;
            int current = node;
            loop.push_back(index_to_name[current]);
            while(inheritance_graph[current][0] != node)
            {
                current = inheritance_graph[current][0];
                loop.push_back(index_to_name[current]);
            }

            inheritance_loops.push_back(loop);
        }
    }
}
//used for dfs_check_features_overriding
unordered_map<string, stack<int>> method_stack;
unordered_map<string, stack<int>> attribute_stack;


void dfs_check_fetures_overriding(vector<vector<int>>& inheritance_graph, int node, vector<bool>& visited, vector<string>& errors)
{
    visited[node] = true;
    auto class_methods_names = class_methods[node].get_names();
    for (int i = 0; i < class_methods_names.size(); i++)
    {
        string method_name = class_methods_names[i];
        vector<int> method_signature = *(class_methods[node].get_signature(method_name));
        if (method_stack.find(method_name) == method_stack.end())
            method_stack[method_name].push(node);
        else
        {
            int ancestor_class_index = method_stack[method_name].top();
            vector<int> ancestor_signature_opt = *(class_methods[ancestor_class_index].get_signature(method_name));
            if(ancestor_signature_opt != method_signature)
                errors.push_back("Override for method " + method_name + " in class " + index_to_name[node] + " has different signature than method in ancestor " + index_to_name[ancestor_class_index] + " (earliest ancestor that mismatches)");
        }
    }

    auto class_attributes_names = class_attributes[node].get_names();
    for(int i = 0; i < class_attributes_names.size(); i++)
    {
        string attribute_name = class_attributes_names[i];
        if(attribute_stack.find(attribute_name) == attribute_stack.end())
            attribute_stack[attribute_name].push(node);
        else 
            errors.push_back("Attribute `" + attribute_name + "` in class `" +
                               index_to_name[node]+
                               "` redefines attribute with the same name in ancestor `" +
                               index_to_name[attribute_stack[attribute_name].top()] + "` (earliest ancestor that defines this attribute)");
    }

    for (auto inherited_method_name: class_methods[inheritance_graph[node][0]].get_names()){
        if (!class_methods[node].contains(inherited_method_name)){
            vector<int> method_signature = *(class_methods[inheritance_graph[node][0]].get_signature(inherited_method_name));
            class_methods[node].add_method(Method(inherited_method_name, method_signature));
        }
    }

    for (auto inherited_attribute_name: class_attributes[inheritance_graph[node][0]].get_names()){
        const string & attribute_name = inherited_attribute_name;
        if (!class_attributes[node].contains(attribute_name)){
            const int attr_type = *class_attributes[inheritance_graph[node][0]].get_type(attribute_name);
            class_attributes[node].add(Attribute(attribute_name, attr_type));
        }
    }

    for (int i = 0; i < inheritance_graph[node].size(); i++)
    {
        int to = inheritance_graph[node][i];
        if (!visited[to])
            dfs_check_fetures_overriding(inheritance_graph, to, visited, errors);
    }

    for (int i = 0; i < class_methods_names.size(); ++i)
    {
        string method_name = class_methods_names[i];
        if (!method_stack[method_name].empty() && method_stack[method_name].top() == node)
            method_stack[method_name].pop();
    }

    for (int i = 0; i < class_attributes_names.size(); ++i)
    {
        string attribute_name = class_attributes_names[i];
        if (!attribute_stack[attribute_name].empty() && attribute_stack[attribute_name].top() == node)
            attribute_stack[attribute_name].pop();
         if(attribute_stack[attribute_name].empty())
            attribute_stack.erase(attribute_name);
    }


}

string print_inheritance_loops_error(vector<vector<string>> inheritance_loops);

// Runs semantic analysis and returns a list of errors, if any.
//
// TODO: change the type from void * to your typed AST type

inline void add_object_methods() 
{
    class_methods[name_to_index["Object"]] = Methods();

    vector<string> arg_names;

    // abort() : Object
    vector<int> abort_signature;
    abort_signature.push_back(name_to_index["Object"]);
    class_methods[name_to_index["Object"]].add_method(
        Method("abort", abort_signature)
    );
    class_methods[name_to_index["Object"]].set_argument_names("abort", arg_names);

    // type_name() : String
    vector<int> type_name_signature;
    type_name_signature.push_back(name_to_index["String"]);
    class_methods[name_to_index["Object"]].add_method(
        Method("type_name", type_name_signature)
    );
    class_methods[name_to_index["Object"]].set_argument_names("type_name", arg_names);

    // copy() : SELF_TYPE
    vector<int> copy_signature;
    copy_signature.push_back(name_to_index["SELF_TYPE"]);
    class_methods[name_to_index["Object"]].add_method(
        Method("copy", copy_signature)
    );
    class_methods[name_to_index["Object"]].set_argument_names("copy", arg_names);
}

inline void add_IO_methods()
{
class_methods[name_to_index["IO"]] = Methods();

    vector<string> arg_names;

    // out_string(x : String) : SELF_TYPE
    vector<int> out_string_signature;
    out_string_signature.push_back(name_to_index["String"]);     // argument
    out_string_signature.push_back(name_to_index["SELF_TYPE"]);  // return type
    class_methods[name_to_index["IO"]].add_method(
        Method("out_string", out_string_signature)
    );
    arg_names.push_back("x");
    class_methods[name_to_index["IO"]].set_argument_names("out_string", arg_names);

    // out_int(x : Int) : SELF_TYPE
    arg_names.clear();
    vector<int> out_int_signature;
    out_int_signature.push_back(name_to_index["Int"]);            
    out_int_signature.push_back(name_to_index["SELF_TYPE"]);       
    class_methods[name_to_index["IO"]].add_method(
        Method("out_int", out_int_signature)
    );
    arg_names.push_back("x");
    class_methods[name_to_index["IO"]].set_argument_names("out_int", arg_names);

    // in_string() : String
    arg_names.clear();
    vector<int> in_string_signature;
    in_string_signature.push_back(name_to_index["String"]);
    class_methods[name_to_index["IO"]].add_method(
        Method("in_string", in_string_signature)
    );
    class_methods[name_to_index["IO"]].set_argument_names("in_string", arg_names);

    // in_int() : Int
    vector<int> in_int_signature;
    in_int_signature.push_back(name_to_index["Int"]);
    class_methods[name_to_index["IO"]].add_method(
        Method("in_int", in_int_signature)
    );
    class_methods[name_to_index["IO"]].set_argument_names("in_int", arg_names);
  
}

inline void add_string_methods(){
    /*
        length() : Int
        concat(s : String) : String
        substr(i : Int, l : Int) : String
    */
    class_methods[name_to_index["String"]] = Methods();
    vector<int> length_signature;
    length_signature.push_back(name_to_index["Int"]);
    class_methods[name_to_index["String"]].add_method(Method("length", length_signature));
    vector<string> arg_names;
    class_methods[name_to_index["String"]].set_argument_names("length", arg_names);
    vector<int> concat_signature;
    concat_signature.push_back(name_to_index["String"]);
    concat_signature.push_back(name_to_index["String"]);
    class_methods[name_to_index["String"]].add_method(Method("concat", concat_signature));
    //arg_names.clear();
    arg_names.push_back("s");
    class_methods[name_to_index["String"]].set_argument_names("concat", arg_names);
    vector<int> substr_signature;
    substr_signature.push_back(name_to_index["Int"]);
    substr_signature.push_back(name_to_index["Int"]);
    substr_signature.push_back(name_to_index["String"]);
    class_methods[name_to_index["String"]].add_method(Method("substr", substr_signature));
    arg_names.clear();
    arg_names.push_back("i");
    arg_names.push_back("l");
    class_methods[name_to_index["String"]].set_argument_names("substr", arg_names);
}

expected<void *, vector<string>> CoolSemantics::run() {

    vector<string> errors;

    // collect classes

    CoolParser::ProgramContext* programCtx = parser_->program();
    auto classes = programCtx->class_();

    vector<string> buildin_types = {"Object", "IO", "Int", "Bool", "String"};
    int count = 0;
    for (; count < buildin_types.size(); count++)
    {
        name_to_index[buildin_types[count]] = count;
        index_to_name.push_back(buildin_types[count]);
    }

    for( auto c : classes ) {
        string &&class_name = c->TYPEID(0)->getText();
        if(name_to_index.find(class_name) != name_to_index.end()){
            errors.push_back("Type `" + class_name + "` already defined");
        }
        else 
        {
            name_to_index[class_name] = count;
            index_to_name.push_back(class_name);
            count++;
        }
    }

    const int SELF_TYPE_INT = index_to_name.size();
    name_to_index["SELF_TYPE"] = SELF_TYPE_INT;
    index_to_name.push_back("SELF_TYPE");
    const int NO_TYPE_INT = index_to_name.size();
    name_to_index["NO_TYPE"] = NO_TYPE_INT;
    index_to_name.push_back("NO_TYPE");

    if (!errors.empty()) {
        return unexpected(errors);
    }
    
    // build inheritance graph}
    vector<vector<int>> inheritance_graph(name_to_index.size());

    /*for (int i = 0; i < buildin_types.size(); i++)
        inheritance_graph[i].push_back(0);*/

    for (int i = 1; i < buildin_types.size(); i++){
        inheritance_graph[i].push_back(0);
        inheritance_graph[0].push_back(i);
    }

    for(auto c : classes)
        if (c->TYPEID(0)->getText() != "Object")
            inheritance_graph[name_to_index[c->TYPEID(0)->getText()]].push_back(0);

    bool not_allowed_inheritance = false;
    for( auto c : classes )
    {
        string child = c->TYPEID(0)->getText();
        if (child == "Object") continue;
        if(c->INHERITS())
        {
            string parent = c->TYPEID(1)->getText();
            
            if( name_to_index.find(parent) == name_to_index.end())
                errors.push_back(child + " inherits from undefined class " + parent);
            //TODO check for inheritance from SELF_TYPE
            else if (parent == "SELF_TYPE"){
                //Actually, there are no tests for that check. But still, we can add it.
                //P.S. Update the tests for the next edition of the coursework.
                errors.push_back("Class " + child + " cannot inherit from SELF_TYPE");
                inheritance_graph[0].push_back(name_to_index[child]);
                inheritance_graph[name_to_index[child]][0] = 0;
            }
            else if (parent == "Int" || parent == "String" || parent == "Bool"){
                errors.push_back("`" + child + "` inherits from `" + parent + "` which is an error");
                inheritance_graph[0].push_back(name_to_index[child]);
                inheritance_graph[name_to_index[child]][0] = 0;
                not_allowed_inheritance = true;
            }
            else
            {
                inheritance_graph[name_to_index[parent]].push_back(name_to_index[child]);
                inheritance_graph[name_to_index[child]][0] = name_to_index[parent];
            }
        }
        else 
            inheritance_graph[0].push_back(name_to_index[child]);
    }

    if (not_allowed_inheritance){
        return unexpected(errors);
    }

    // check inheritance graph is a tree

    vector<bool> visited(name_to_index.size() + 1, false);
    vector<int> parent(name_to_index.size() + 1, -1);
    vector<vector<string>> inheritance_loops;



    for(int node = 0; node < inheritance_graph.size(); node++)
        if(!visited[node])
            dfs_detecting_class_loops(inheritance_graph, node, -1, visited, inheritance_loops);

    if(inheritance_loops.size() > 0)
    {
        errors.push_back(print_inheritance_loops_error(inheritance_loops));
    }
    
    //parser_->reset();
    // collect features
    
    add_object_methods();
    add_string_methods();
    add_IO_methods();
    
    for (auto c: classes)
    {
        string class_name = c->TYPEID(0)->getText();
        int class_index = name_to_index[class_name];

        auto methods = c->method();
        for (auto m : methods)
        {
            string method_name = m->OBJECTID()->getText();
            if (class_methods[class_index].contains(method_name))
                errors.push_back("Method `" + method_name + "` already defined for class `" + class_name +'`');
            else
            {
                auto formals = m->formal();
                vector<int> signature;
                vector<string> argument_names;
                bool has_error = false;
                for (auto f : formals)
                {
                    string formal_type = f->TYPEID()->getText();
                    if (formal_type == "SELF_TYPE"){
                        has_error = true;
                        //Formal argument `x` declared of type `SELF_TYPE` which is not allowed
                        errors.push_back("Formal argument `" + f->OBJECTID()->getText() + "` declared of type `SELF_TYPE` which is not allowed");
                    }
                    else if (name_to_index.find(formal_type) == name_to_index.end()){
                        has_error = true;
                        errors.push_back("Method `" + method_name + "` in class `" + class_name + "` declared to have an argument of type `" + formal_type + "` which is undefined");
                    }
                    else{
                        signature.push_back(name_to_index[formal_type]);
                        argument_names.push_back(f->OBJECTID()->getText());
                    }
                }
                
                if (has_error) continue;
                string return_type = m->TYPEID()->getText();
                if(name_to_index.find(return_type) == name_to_index.end())
                    errors.push_back("Method `" + method_name + "` in class `" + class_name + "` declared to have return type `" + return_type + "` which is undefined");
                else{
                    signature.push_back(name_to_index[return_type]);
                    if (formals.size() != signature.size() - 1)
                        errors.push_back("Method `" + method_name + "` in class `" + class_name + "` declared with mismatched number of arguments and signature");
                    else{
                        class_methods[class_index].add_method(Method(method_name, signature));
                        class_methods[class_index].set_argument_names(method_name, argument_names);
                    }
                }
            }
        }

        auto attributes = c->attr();
        for (auto a : attributes)
        {
            string attr_name = a->OBJECTID()->getText();
            string attr_type = a->TYPEID()->getText();

            if (name_to_index.find(attr_type) == name_to_index.end())
                errors.push_back("Attribute `" + attr_name + "` in class `" + class_name + "` declared to have type `" + attr_type + "` which is undefined");
            else if (class_attributes[class_index].contains(attr_name))
                errors.push_back("Attribute `" + attr_name + "` already defined for class `" + class_name + '`');
            else{
                class_attributes[class_index].add(Attribute(attr_name, name_to_index[attr_type]));
                count_class_own_attributes[class_index]++;
            }
        }
    }
   
    // check methods are overridden correctly
    for (int i = 0; i < visited.size(); i++) visited[i] = false;
    dfs_check_fetures_overriding(inheritance_graph, 0, visited, errors);


    LCA lca_finder;
    lca_finder.init(inheritance_graph, 0);
    parser_->reset();

    for (const auto &error : TypeChecker(name_to_index, index_to_name, class_methods, class_attributes,
                                         count_class_own_attributes, SELF_TYPE_INT, NO_TYPE_INT, lca_finder).check(parser_)) {
        errors.push_back(error);
    }

    if (!errors.empty()) {
        return unexpected(errors);
    }

    print_inheritance_loops_error(inheritance_loops);

    // return the typed AST
    return nullptr;
}

string print_inheritance_loops_error(vector<vector<string>> inheritance_loops) {
    stringstream eout;
    eout << "Detected " << inheritance_loops.size()
         << " loops in the type hierarchy:" << endl;
    for (int i = 0; i < inheritance_loops.size(); ++i) {
        eout << i + 1 << ") ";
        auto &loop = inheritance_loops[i];
        for (string name : loop) {
            eout << name << " <- ";
        }
        eout << endl;
    }

    return eout.str();
}