#ifndef CODEGEN_COOL_CODEGEN_H_
#define CODEGEN_COOL_CODEGEN_H_

#include <memory>
#include <ostream>
#include <string>
#include <vector>
#include <unordered_map>

#include "CoolParser.h"
#include "semantics/ClassTable.h"

//include all typed-ast headers
/*
Arithmetic.h  Attributes.h       CaseOfEsac.h          Expr.h          IntegerComparison.h  LetIn.h             Methods.h          ParenthesizedExpr.h  StringConstant.h
Assignment.h  BoolConstant.h     DynamicDispatch.h     IfThenElseFi.h  IntegerNegation.h    Method.h            NewObject.h        Sequence.h           Vardecl.h
Attribute.h   BooleanNegation.h  EqualityComparison.h  IntConstant.h   IsVoid.h             MethodInvocation.h  ObjectReference.h  StaticDispatch.h     WhileLoopPool.h

*/
#include "../semantics/typed-ast/Arithmetic.h"
#include "../semantics/typed-ast/Assignment.h"
#include "../semantics/typed-ast/Attribute.h"
#include "../semantics/typed-ast/Attributes.h"
#include "../semantics/typed-ast/BoolConstant.h"
#include "../semantics/typed-ast/BooleanNegation.h"
#include "../semantics/typed-ast/CaseOfEsac.h"
#include "../semantics/typed-ast/DynamicDispatch.h"
#include "../semantics/typed-ast/EqualityComparison.h"
#include "../semantics/typed-ast/Expr.h"
#include "../semantics/typed-ast/IfThenElseFi.h"
#include "../semantics/typed-ast/IntConstant.h"
#include "../semantics/typed-ast/IntegerComparison.h"
#include "../semantics/typed-ast/IntegerNegation.h"
#include "../semantics/typed-ast/IsVoid.h"
#include "../semantics/typed-ast/LetIn.h"
#include "../semantics/typed-ast/Method.h"
#include "../semantics/typed-ast/MethodInvocation.h"
#include "../semantics/typed-ast/Methods.h"
#include "../semantics/typed-ast/NewObject.h"
#include "../semantics/typed-ast/ObjectReference.h"
#include "../semantics/typed-ast/ParenthesizedExpr.h"
#include "../semantics/typed-ast/Sequence.h"
#include "../semantics/typed-ast/StaticDispatch.h"
#include "../semantics/typed-ast/StringConstant.h"
#include "../semantics/typed-ast/Vardecl.h"
#include "../semantics/typed-ast/WhileLoopPool.h"

using namespace std;

struct mem_place{
    /*
    offset: offset in bytes from the start of the object
    isattr = true: search with the offset from the self instance
    isattr = false: search with the offset from $fp (for local vars)
    */
    int offset;
    bool isattr;
    mem_place(){}
    mem_place(int _offset, bool _isattr): offset(_offset), isattr(_isattr) {}
    bool is_attr() const { return isattr; }
    int get_offset() const { return offset; }
    bool operator==(const mem_place &other) const {
        return offset == other.offset && isattr == other.isattr;
    }
};

struct static_consts{
  /*int count_int;
  int count_string;
  int count_bool;*/
  // maps from constant value to label
  unordered_map<int, string> int_consts;
  unordered_map<string, string> string_consts;
  unordered_map<bool, string> bool_consts;
  //static_consts(): count_int(0), count_string(0), count_bool(0){}
  static_consts(){}
  void add_string(const string &s){
      // add code to create prototype object and init method for the class
    if (string_consts.find(s) == string_consts.end()){
        int count = string_consts.size();
        string label = "_string_const_" + to_string(count);
        string_consts[s] = label;
    }
  }

  void add_int(int i){
      // add code to create prototype object and init method for the class
    if (int_consts.find(i) == int_consts.end()){
        int count = int_consts.size();
        string label = "_int_const_" + to_string(count);
        int_consts[i] = label;
    }
  }

  void add_bool(bool b){
      // add code to create prototype object and init method for the class
    if (bool_consts.find(b) == bool_consts.end()){
        int count = bool_consts.size();
        string label = "_bool_const_" + to_string(count);
        bool_consts[b] = label;
    }
  }
};

/*used for case-of-esac code generation
sort the ranges by sub-hierarchy size ascending
*/
struct class_range{
  int class_index;
  int sub_hierarchy_size;
  int case_branch_index;
  class_range() = default;
  class_range(int _class_index, int _sub_hierarchy_size, int _case_branch_index)
    : class_index(_class_index),
      sub_hierarchy_size(_sub_hierarchy_size),
      case_branch_index(_case_branch_index) {}
  bool operator<(const class_range &other) const {
      return sub_hierarchy_size < other.sub_hierarchy_size;
  }
};

class CoolCodegen {
  private:
    std::string file_name_;
    std::unique_ptr<ClassTable> class_table_;
    vector<bool> built_classes;//(num_classes, false);
    vector<unordered_map<string, mem_place>> class_attr_locations;//(num_classes);
    vector<string> dispatch_table_labels;//(num_classes);
    vector<int> class_sizes;//(num_classes, 0);
    unordered_map<string, stack<mem_place>> local_var_locations;//for vars(local and attributes) in methods
    static_consts code_consts;
    int label_count;
    int negative_offset_fp;
    string label_base = "label_";
    int in_class;
    int if_then_else_count;
    int stack_pointer_pos;
    int while_loop_count;
    int case_count;
  public:
    CoolCodegen(std::string file_name, std::unique_ptr<ClassTable> class_table)
        : file_name_(std::move(file_name)),
          class_table_(std::move(class_table)) {
            label_count = 0;
            if_then_else_count = 0;
            negative_offset_fp = 0;
            in_class = -1;
            stack_pointer_pos = 0;
            while_loop_count = 0;
            case_count = 0;
      }
    int emitted_string_length(const std::string& s);
    void generate(std::ostream &out);
    void build_class(int idx/*, 
                     /*std::vector<bool> &built_classes,
                     std::vector<std::unordered_map<std::string, mem_place>> &class_attr_locations,
                     std::vector<std::string> &dispatch_table_labels,
                     std::vector<int> &class_sizes*/);
    void Object_init(std::ostream &out, string label);
    void String_init(std::ostream &out);
    bool is_basic_class(const string& class_name);
    //void is_basic_method_call(std::ostream &out, string class_name, string method_name);
    void generate_call_Object_copy(std::ostream &out);
    void generate_call_init(std::ostream &out, string &class_name);
    void generate_method_in(std::ostream &out);
    void generate_method_out(std::ostream &out);
    void generate_method_call(std::ostream &out, int idx, int defining_class_index, const string &class_name, const string &method_name);
    void generate_expr_code(std::ostream &out, const Expr* expr);
    void generate_static_dispatch_code(std::ostream &out, const StaticDispatch* st_dispatch);
    void generate_dynamic_dispatch_code(std::ostream &out, const DynamicDispatch* dy_dispatch);
    void generate_method_invoke(std::ostream &out, const MethodInvocation* method_invocation);
    void generate_method_arguments_set(std::ostream &out, unordered_map<string, mem_place> &attr_locations,
                                      vector<string> &args_names, vector<Expr*> &args_exprs,
                                      string &dispatch_class_name, string &method_name);
    /*Emits code to increment sp register by 4 and increment stack_pointer_pos by 4*/
    void print_incr_stack_pointer_4(std::ostream &out);
    /*Emits code to increment sp register by 12 and increment stack_pointer_pos by 12*/
    void print_incr_stack_pointer_12(std::ostream &out);
    /*Emits code to increment sp register by n and increment stack_pointer_pos by n. MUST: n % 4 == 0 */
    void print_incr_stack_pointer_n(std::ostream &out, int n);
    /*Emits code to decrement sp register by 4 and decrement stack_pointer_pos by 4*/
    void print_decr_stack_pointer_4(std::ostream &out);
    /*Emits code to decrement sp register by 12 and decrement stack_pointer_pos by 12*/
    void print_decr_stack_pointer_12(std::ostream &out);
    /*Emits code to decrement sp register by n and decrement stack_pointer_pos by n. MUST: n % 4 == 0 */
    void print_decr_stack_pointer_n(std::ostream &out, int n);
    /*Only increments, NO emitting code. Useful for restoring stack
    pointer position after calling a function. Simulation of popping arguments
    MUST n % 4 == 0*/  
    void only_incr_stack_pointer_pos_n(int n){
        assert(n % 4 == 0);
        stack_pointer_pos += n;
    }
    /*Only decrements, NO emitting code. Useful for restoring stack
    pointer position after calling a function. Simulation of popping arguments
    MUST n % 4 == 0. Maybe never used*/
    void only_decr_stack_pointer_pos_n(int n){
        assert(n % 4 == 0);
        stack_pointer_pos -= n;
    }

    //void insert_attr_to
};

#endif
