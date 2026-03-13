#ifndef SEMANTICS_PASSES_TYPE_CHECKER_H_
#define SEMANTICS_PASSES_TYPE_CHECKER_H_

#include <any>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <stack>
#include <unordered_set>

#include "LCA.h"
#include "CoolParser.h"
#include "CoolParserBaseVisitor.h"
#include <Methods.h>
#include <Attributes.h>
using namespace std;

//const int SELF_TYPE_INT = (1<<30);
//const int NO_TYPE_INT = (1<<30) | 1;

class TypeChecker : public CoolParserBaseVisitor {
  private:
    // define any necessary fields
    std::vector<std::string> errors;
    unordered_map<string, stack<int>> var_stack;// environment O
    int current_class_index; // to know which class we are in

    unordered_map<string,int> & type_name_to_index;
    vector<string> & index_to_type_name;

    unordered_map<int, Methods> & class_methods;//name_to_index --> int // environment M
    unordered_map<int, Attributes> & class_attributes;
    unordered_map<int, int> &count_class_own_attributes;
    
    LCA lca_finder;
    // override necessary visitor methods

    // define helper methods
    const int SELF_TYPE_INT;
    const int NO_TYPE_INT;

  public:
    // TODO: add necessary dependencies to constructor
    TypeChecker(unordered_map<string,int> & _type_name_to_index,
                vector<string> & _index_to_type_name,
                unordered_map<int, Methods> & _class_methods,
                unordered_map<int, Attributes> & _class_attributes,
                unordered_map<int, int> & _count_class_own_attributes,
                const int _SELF_TYPE_INT,
                const int _NO_TYPE_INT,
                LCA & _lca_finder)
        : type_name_to_index(_type_name_to_index),
          index_to_type_name(_index_to_type_name),
          class_methods(_class_methods),
          class_attributes(_class_attributes),
          count_class_own_attributes(_count_class_own_attributes),
          lca_finder(_lca_finder),
          current_class_index(-1),
          SELF_TYPE_INT(_SELF_TYPE_INT),
          NO_TYPE_INT(_NO_TYPE_INT) {};

    // Typechecks the AST that the parser produces and returns a list of errors,
    // if any.
    std::vector<std::string> check(CoolParser *parser);

    std::any visitClass(CoolParser::ClassContext *ctx) override;

    std::any visitExpr(CoolParser::ExprContext *ctx) override;
    void check_class_methods(CoolParser::ClassContext *ctx);
};

#endif