#include <cctype>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include "antlr4-runtime/antlr4-runtime.h"

#include "CoolLexer.h"
#include "CoolParser.h"
#include "CoolParserBaseVisitor.h"

using namespace std;
using namespace antlr4;

namespace fs = filesystem;

/// Converts token to coursework-specific string representation.
string cool_token_to_string(CoolLexer *lexer, Token *token) {
    auto token_type = token->getType();

    // clang-format off
    switch (token_type) {
        case static_cast<size_t>(-1) : return "EOF";

        case CoolLexer::SEMI   : return "';'";
        case CoolLexer::OCURLY : return "'{'";
        case CoolLexer::CCURLY : return "'}'";
        case CoolLexer::OPAREN : return "'('";
        case CoolLexer::COMMA  : return "','";
        case CoolLexer::CPAREN : return "')'";
        case CoolLexer::COLON  : return "':'";
        case CoolLexer::AT     : return "'@'";
        case CoolLexer::DOT    : return "'.'";
        case CoolLexer::PLUS   : return "'+'";
        case CoolLexer::MINUS  : return "'-'";
        case CoolLexer::STAR   : return "'*'";
        case CoolLexer::SLASH  : return "'/'";
        case CoolLexer::TILDE  : return "'~'";
        case CoolLexer::LT     : return "'<'";
        case CoolLexer::EQ     : return "'='";

        case CoolLexer::DARROW : return "DARROW";
        case CoolLexer::ASSIGN : return "ASSIGN";
        case CoolLexer::LE     : return "LE";

        case CoolLexer::CLASS  : return "CLASS";
        case CoolLexer::ELSE   : return "ELSE";
        case CoolLexer::FI     : return "FI";
        case CoolLexer::IF     : return "IF";
        case CoolLexer::IN     : return "IN";
        case CoolLexer::INHERITS : return "INHERITS";
        case CoolLexer::ISVOID : return "ISVOID";
        case CoolLexer::LET    : return "LET";
        case CoolLexer::LOOP   : return "LOOP";
        case CoolLexer::POOL   : return "POOL";
        case CoolLexer::THEN   : return "THEN";
        case CoolLexer::WHILE  : return "WHILE";
        case CoolLexer::CASE   : return "CASE";
        case CoolLexer::ESAC   : return "ESAC";
        case CoolLexer::NEW    : return "NEW";
        case CoolLexer::OF     : return "OF";
        case CoolLexer::NOT    : return "NOT";

        case CoolLexer::BOOL_CONST : return "BOOL_CONST";
        case CoolLexer::INT_CONST  : return "INT_CONST = " + token->getText();
        case CoolLexer::STR_CONST  : return "STR_CONST";
        case CoolLexer::TYPEID     : return "TYPEID = " + token->getText();
        case CoolLexer::OBJECTID   : return "OBJECTID = " + token->getText();
        case CoolLexer::ERROR      : return "ERROR";

        default : return "<Invalid Token>: " + token->toString();
    }
    // clang-format on
}

class TreePrinter : public CoolParserBaseVisitor {
  private:
    CoolLexer *lexer_;
    CoolParser *parser_;
    string file_name_;
    string indent = "";
    void inline increase_indent() { indent += "  "; }
    void inline decrease_indent() { indent.pop_back(); indent.pop_back(); }

  public:


    TreePrinter(CoolLexer *lexer, CoolParser *parser, const string &file_name)
        : lexer_(lexer), parser_(parser), file_name_(file_name) {}

    any visitProgram(CoolParser::ProgramContext *ctx) override {
        cout << '#' << ctx->getStop()->getLine() << endl;
        cout << "_program" << endl;
        increase_indent();
        visitChildren(ctx);
        decrease_indent();

        return any{};
    }

    any visitClass(CoolParser::ClassContext *ctx) override {
        cout << indent << '#' << ctx->getStop()->getLine() << endl;
        cout << indent << "_class" << endl;
        cout << indent << "  " << ctx->TYPEID(0)->getText() << endl;
        if (ctx->INHERITS()) cout << indent << "  " << ctx->TYPEID(1)->getText() << endl;
        else cout << indent << "  " << "Object" << endl;
        cout << indent << "  \"" << file_name_ << '"' << endl;
        cout << indent << "  " << '(' << endl;
        increase_indent();
        visitChildren(ctx);
        decrease_indent();
        cout << indent << "  " << ')' << endl;
        return any{};
    }

    any visitObjectid_expr(CoolParser::Objectid_exprContext *ctx) override {
        cout << indent << "#" << ctx->getStop()->getLine() << endl;
        cout << indent << "_object" << endl;
        cout << indent << "  " << ctx->OBJECTID()->getText() << endl;
        cout << indent << ": _no_type" << endl;
        return any{};
    }

    any visitMethod(CoolParser::MethodContext *ctx) override {
        cout << indent << "#" << ctx->getStop()->getLine() << endl;
        cout << indent << "_method" << endl;
        cout << indent << "  " << ctx->OBJECTID()->getText() << endl;
        //print formals
        increase_indent();
        for (auto formalCtx : ctx->formal()) {
            visit(formalCtx);
        }
        decrease_indent();
        // return type
        cout << indent << "  " << ctx->TYPEID()->getText() << endl;
        if (ctx->expr()) {
            increase_indent();
            visit(ctx->expr());
            decrease_indent();
        }
        return any{};
    }

    any visitAttr(CoolParser::AttrContext *ctx) override {
        cout << indent << "#" << ctx->getStop()->getLine() << endl;
        cout << indent << "_attr" << endl;
        cout << indent << "  " << ctx->OBJECTID()->getText() << endl;
        cout << indent << "  " << ctx->TYPEID()->getText() << endl;
        increase_indent();
        // Check if there is an initializer ("<- expr")
        if (ctx->expr() == nullptr) {
            // No initializer: print _no_expr and : _no_type
            cout << indent << "#" << ctx->getStop()->getLine() << endl;
            cout << indent << "_no_expr" << endl;
            cout << indent << ": _no_type" << endl;
        }
        else visitChildren(ctx);
        decrease_indent();

        return any{};
    }

    any visitFormal(CoolParser::FormalContext *ctx) override {
        cout << indent << "#" << ctx->getStop()->getLine() << endl;
        cout << indent << "_formal" << endl;
        cout << indent << "  " << ctx->OBJECTID()->getText() << endl;
        cout << indent << "  " << ctx->TYPEID()->getText() << endl;
        return any{};
    }

    any visitAssign_expr(CoolParser::Assign_exprContext *ctx) override {
        cout << indent << "#" << ctx->getStop()->getLine() << endl;
        cout << indent << "_assign" << endl;
        cout << indent << "  " << ctx->OBJECTID()->getText() << endl;
        increase_indent();
        visitChildren(ctx);
        decrease_indent();
        cout << indent << ": _no_type" << endl;
        return any{};
    }

    any visitStatic_dispatch(CoolParser::Static_dispatchContext *ctx) override {
        cout << indent << "#" << ctx->getStop()->getLine() << endl;
        cout << indent << "_static_dispatch" << endl;
        vector<CoolParser::ExprContext *> exprs = ctx->expr();
        increase_indent();
        visit(exprs[0]);
        decrease_indent();
        // print the TYPEID if present
        if (ctx->TYPEID()) 
            cout << indent << "  " << ctx->TYPEID()->getText() << endl;
        else cout << indent << "  " << "_no_type" << endl;
        // print the method name
        cout << indent << "  " << ctx->OBJECTID()->getText() << endl;
        cout << indent << "  " << '(' << endl;
        increase_indent();
        // visit the actual parameters
        for (size_t i = 1; i < exprs.size(); ++i) {
            visit(exprs[i]);
        }
        decrease_indent();
        cout << indent << "  " << ')' << endl;
        cout << indent << ": _no_type" << endl;
        return any{};
    }

    any visitFunction_call_expr(CoolParser::Function_call_exprContext *ctx) override {
        cout << indent << "#" << ctx->getStop()->getLine() << endl;
        cout << indent << "_dispatch" << endl;
        //print it like static dispatch but without TYPEID and with self
        cout << indent << "  #" << ctx->getStop()->getLine() << endl;
        cout << indent << "  " << "_object" << endl;
        cout << indent << "    self" << endl;
        cout << indent << "  : _no_type" << endl;
        // print method name
        cout << indent << "  " << ctx->OBJECTID()->getText() << endl;
        cout << indent << "  " << '(' << endl;
        increase_indent();
        for (auto exprCtx : ctx->expr()) {
            visit(exprCtx);
        }
        decrease_indent();
        cout << indent << "  " << ')' << endl;
        cout << indent << ": _no_type" << endl;
        return any{};
    }

    any visitIf_expr(CoolParser::If_exprContext *ctx) override {
        cout << indent << "#" << ctx->getStop()->getLine() << endl;
        cout << indent << "_cond" << endl;
        increase_indent();
        /*visit(ctx->expr(0)); // condition
        visit(ctx->expr(1)); // then branch
        visit(ctx->expr(2)); // else branch*/
        for (auto exprCtx : ctx->expr())
            visit(exprCtx);
        decrease_indent();
        cout << indent << ": _no_type" << endl;
        return any{};
    }

    any visitWhile_expr(CoolParser::While_exprContext *ctx) override {
        cout << indent << "#" << ctx->getStop()->getLine() << endl;
        cout << indent << "_loop" << endl;
        increase_indent();
        visitChildren(ctx);
        decrease_indent();
        cout << indent << ": _no_type" << endl;
        return any{};
    }

    any visitBlock_expr(CoolParser::Block_exprContext *ctx) override {
        cout << indent << "#" << ctx->getStop()->getLine() << endl;
        cout << indent << "_block" << endl;
        increase_indent();
        visitChildren(ctx);
        decrease_indent();
        cout << indent << ": _no_type" << endl;
        return any{};
    }

    any visitLet_var(CoolParser::Let_varContext *ctx) override {
        cout << indent << "#" << ctx->getStop()->getLine() << endl;
        cout << indent << "_let" << endl;
        cout << indent << "  " << ctx->OBJECTID()->getText() << endl;
        cout << indent << "  " << ctx->TYPEID()->getText() << endl;
        if (ctx->expr() == nullptr) {
            cout << indent << "  #" << ctx->getStop()->getLine() << endl;
            cout << indent << "  _no_expr" << endl;
            cout << indent << "  : _no_type" << endl;
        }
        else {
            increase_indent();
            visit(ctx->expr());
            decrease_indent();
        }
        return any{};
    }

    inline void print_let_var(CoolParser::Let_varContext *ctx, int line_number){
        cout << indent << "#" << line_number << endl;
        cout << indent << "_let" << endl;
        cout << indent << "  " << ctx->OBJECTID()->getText() << endl;
        cout << indent << "  " << ctx->TYPEID()->getText() << endl;
        if (ctx->expr() == nullptr) {
            cout << indent << "  #" << ctx->getStop()->getLine() << endl;
            cout << indent << "  _no_expr" << endl;
            cout << indent << "  : _no_type" << endl;
        }
        else {
            increase_indent();
            visit(ctx->expr());
            decrease_indent();
        }
    }

    any visitLet_expr(CoolParser::Let_exprContext *ctx) override {
        size_t count_let_vars = ctx->let_var().size();
        int line_number = ctx->getStop()->getLine();

        for (size_t i = 0; i < count_let_vars; ++i) {
            //visit(ctx->let_var(i));
            print_let_var(ctx->let_var(i), line_number);
            increase_indent();
        }
        visit(ctx->expr());
        for (size_t i = 0; i < count_let_vars; ++i) {
            decrease_indent();
            cout << indent << ": _no_type" << endl;
        }
        return any{};
    }

    any visitInt_const_expr(CoolParser::Int_const_exprContext *ctx) override {
        cout << indent << "#" << ctx->getStop()->getLine() << endl;
        cout << indent << "_int" << endl;
        cout << indent << "  " << ctx->INT_CONST()->getText() << endl;
        cout << indent << ": _no_type" << endl;
        return any{};
    }
    
    any visitBool_const_expr(CoolParser::Bool_const_exprContext *ctx) override {
        int char_index = ctx->BOOL_CONST()->getSymbol()->getStartIndex();
        int bool_value = lexer_->get_bool_value(char_index);
        cout << indent << "#" << ctx->getStop()->getLine() << endl;
        cout << indent << "_bool" << endl;
        cout << indent << "  " << bool_value << endl;
        cout << indent << ": _no_type" << endl;
        return any{};
    }

    any visitStr_const_expr(CoolParser::Str_const_exprContext *ctx) override {
        cout << indent << "#" << ctx->getStop()->getLine() << "\n";
        cout << indent << "_string" << "\n";
        cout << indent << "  " << ctx->STR_CONST()->getText() << "\n";
        cout << indent << ": _no_type\n";
        return any{};
    }

    any visitCase_expr(CoolParser::Case_exprContext *ctx) override{
       cout << indent << "#" << ctx->getStop()->getLine() << "\n";
       cout << indent << "_typcase\n"; 
       increase_indent();
       visitChildren(ctx);
       decrease_indent();
       cout << indent << ": _no_type\n";
       return any{};
    }

    any visitCase_var(CoolParser::Case_varContext *ctx) override{
        cout << indent << "#" << ctx->getStop()->getLine() << "\n";
        cout << indent << "_branch\n";
        cout << indent << "  " << ctx->OBJECTID()->getText() << "\n";
        cout << indent << "  " << ctx->TYPEID()->getText() << "\n";
        increase_indent();
        visitChildren(ctx);
        decrease_indent();
        return any{};
    }

    any visitAdditive_expr(CoolParser::Additive_exprContext *ctx) override {
        cout << indent << "#" << ctx->getStop()->getLine() << "\n";
        if (ctx->PLUS()) cout << indent << "_plus\n";
        else cout << indent << "_sub\n";
        increase_indent();
        visitChildren(ctx);
        decrease_indent();
        cout << indent << ": _no_type\n";
        return any{};
    }

    any visitMultiplicative_expr(CoolParser::Multiplicative_exprContext *ctx) override{
        cout << indent << "#" << ctx->getStop()->getLine() << "\n";
        if (ctx->STAR()) cout << indent << "_mul\n";
        else cout << indent << "_divide\n";
        increase_indent();
        visitChildren(ctx);
        decrease_indent();
        cout << indent << ": _no_type\n";
        return any{};
    }

    any visitNegation_expr(CoolParser::Negation_exprContext *ctx) override {
        cout << indent << "#" << ctx->getStop()->getLine() << "\n";
        cout << indent << "_neg\n";
        increase_indent();
        visitChildren(ctx);
        decrease_indent();
        cout << indent << ": _no_type\n";
        return any{};
    }

    any visitComparison_expr(CoolParser::Comparison_exprContext *ctx) override {
        cout << indent << "#" << ctx->getStop()->getLine() << "\n";
        if (ctx->LT()) cout << indent << "_lt\n"; // operator<
        else if (ctx->LE()) cout << indent << "_le\n"; // operator<=
        else cout << indent << "_eq\n"; // operator==
        increase_indent();
        visitChildren(ctx);
        decrease_indent();
        cout << indent << ": _no_type\n";
        return any{};
    }

    any visitDispatch_expr(CoolParser::Dispatch_exprContext *ctx) override {
        cout << indent << "#" << ctx->getStop()->getLine() << endl;
        //cout << indent << "_dispatch_not_static" << endl;
        cout << indent << "_dispatch" << endl;
        vector<CoolParser::ExprContext *> exprs = ctx->expr();
        increase_indent();
        visit(exprs[0]);
        decrease_indent();
        // print the method name
        cout << indent << "  " << ctx->OBJECTID()->getText() << endl;
        cout << indent << "  " << '(' << endl;
        increase_indent();
        // visit the actual parameters
        for (size_t i = 1; i < exprs.size(); ++i)
            visit(exprs[i]);
        decrease_indent();
        cout << indent << "  " << ')' << endl;
        cout << indent << ": _no_type" << endl;
        return any{};
    }
    
    any visitNew_expr(CoolParser::New_exprContext *ctx) override {
        cout << indent << "#" << ctx->getStop()->getLine() << endl;
        cout << indent << "_new" << endl;
        cout << indent << "  " << ctx->TYPEID()->getText() << endl;
        cout << indent << ": _no_type" << endl;
        return any{};
    }

    any visitIsvoid_expr(CoolParser::Isvoid_exprContext *ctx) override {
        cout << indent << "#" << ctx->getStop()->getLine() << endl;
        cout << indent << "_isvoid" << endl;
        increase_indent();
        visitChildren(ctx);
        decrease_indent();
        cout << indent << ": _no_type" << endl;
        return any{};
    }

    any visitNot_expr(CoolParser::Not_exprContext *ctx) override{
        cout << indent << "#" << ctx->getStop()->getLine() << endl;
        cout << indent << "_comp" << endl;
        increase_indent();
        visitChildren(ctx);
        decrease_indent();
        cout << indent << ": _no_type" << endl;
        return any{};
    }


  public:
    void print() { visitProgram(parser_->program()); }
};

class TreeErrorChecker : public CoolParserBaseVisitor {
  // Implement error checking visitor if needed
    private:
        CoolLexer *lexer_;
        CoolParser *parser_;
        string file_name_;
        any visitComparison_expr(CoolParser::Comparison_exprContext *ctx) override {
            string token_str;
            if (ctx->LT()) token_str = "'<'";
            else if (ctx->LE()) token_str = "LE";
            else token_str = "'='";
            for (auto child : ctx->children){
                if (dynamic_cast<CoolParser::Comparison_exprContext*>(child) != nullptr) {
                    cout << '"' << file_name_ << "\", line " << ctx->getStart()->getLine()
                    << ": syntax error at or near "
                    << token_str << endl;
                    cout << "Compilation halted due to lex and parse errors" << endl;
                    exit(0);
                }
            }
            visitChildren(ctx);
            return any{};
        }
    public:
    TreeErrorChecker(CoolLexer *lexer, CoolParser *parser, const string &file_name)
        : lexer_(lexer), parser_(parser), file_name_(file_name) {}
    
    void check() { visitProgram(parser_->program()); }

};

class ErrorPrinter : public BaseErrorListener {
  private:
    string file_name_;
    CoolLexer *lexer_;
    bool has_error_ = false;

  public:
    ErrorPrinter(const string &file_name, CoolLexer *lexer)
        : file_name_(file_name), lexer_(lexer) {}

    virtual void syntaxError(Recognizer *recognizer, Token *offendingSymbol,
                             size_t line, size_t charPositionInLine,
                             const std::string &msg,
                             std::exception_ptr e) override {
        has_error_ = true;
        cout << '"' << file_name_ << "\", line " << line
             << ": syntax error at or near "
             << cool_token_to_string(lexer_, offendingSymbol) << endl;
    }

    bool has_error() const { return has_error_; }
};

int main(int argc, const char *argv[]) {
    if (argc != 2) {
        cerr << "Expecting exactly one argument: name of input file" << endl;
        return 1;
    }

    auto file_path = argv[1];
    ifstream fin(file_path);

    auto file_name = fs::path(file_path).filename().string();

    ANTLRInputStream input(fin);
    CoolLexer lexer(&input);

    CommonTokenStream tokenStream(&lexer);

    CoolParser parser(&tokenStream);

    ErrorPrinter error_printer(file_name, &lexer);

    parser.removeErrorListener(&ConsoleErrorListener::INSTANCE);
    parser.addErrorListener(&error_printer);

    // This will trigger the error_printer, in case there are errors.

    parser.program();
    parser.reset();

    TreeErrorChecker error_tree(&lexer, &parser, file_name);
    error_tree.check();

    parser.reset();
    parser.program();
    parser.reset();

    if (!error_printer.has_error()) {
        TreePrinter(&lexer, &parser, file_name).print();
    } else {
        cout << "Compilation halted due to lex and parse errors" << endl;
    }

    return 0;
}
