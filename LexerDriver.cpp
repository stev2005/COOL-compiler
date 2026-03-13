//Authors: Eli (3MI0600110), Stefan (6MI0800588)
#include <cctype>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "antlr4-runtime/antlr4-runtime.h"

#include "CoolLexer.h"

using namespace std;
using namespace antlr4;

/// Преобразува жетон в текст, който очаква системата за проверка на курсовата работа (част 1).
string cool_token_to_string(Token *token) {
    auto token_type = token->getType();

    switch (token_type) {
        case static_cast<size_t>(-1) : return "EOF";

        // Добавете тук останалите жетони, които представляват само един символ.
        
        case CoolLexer::LPAREN: return "'('";
        case CoolLexer::RPAREN: return "')'";
        case CoolLexer::LBRACE: return "'{'";
        case CoolLexer::RBRACE: return "'}'";
        case CoolLexer::DOT: return "'.'";
        case CoolLexer::ATYSM: return "'@'";
        case CoolLexer::TILDE: return "'~'";
        case CoolLexer::ISVOID: return "ISVOID";
        case CoolLexer::STAR: return "'*'";
        case CoolLexer::SLASH: return "'/'";
        case CoolLexer::PLUS: return "'+'";
        case CoolLexer::MINUS: return "'-'";
        case CoolLexer::LE: return "LE";
        case CoolLexer::LESS: return "'<'";
        case CoolLexer::EQ: return "'='";
        case CoolLexer::ASSIGN: return "ASSIGN";
        case CoolLexer::COMMA: return "','";
        case CoolLexer::SEMI: return "';'";
        case CoolLexer::COLON: return "':'";
        case CoolLexer::DARROW: return "DARROW";
        case CoolLexer::BOOL_CONST: return "BOOL_CONST";
        case CoolLexer::CLASS: return "CLASS";
        case CoolLexer::ELSE: return "ELSE";
        case CoolLexer::FI: return "FI";
        case CoolLexer::IF: return "IF";
        case CoolLexer::IN: return "IN";
        case CoolLexer::NOT: return "NOT";
        case CoolLexer::INHERITS: return "INHERITS";
        case CoolLexer::LET: return "LET";
        case CoolLexer::LOOP: return "LOOP";
        case CoolLexer::POOL: return "POOL";
        case CoolLexer::THEN: return "THEN";
        case CoolLexer::WHILE: return "WHILE";
        case CoolLexer::CASE: return "CASE";
        case CoolLexer::ESAC: return "ESAC";
        case CoolLexer::NEW: return "NEW";
        case CoolLexer::OF: return "OF";

        // Добавете тук останалите валидни жетони (включително и ERROR).
        case CoolLexer::NOT_RECOGNIZED: return "<NOT_RECOGNIZED>: " + token->getText();
        case CoolLexer::ERROR: return "ERROR:";
        case CoolLexer::INT_CONST: return "INT_CONST " + token->getText();
        case CoolLexer::SELF: return "SELF";
        case CoolLexer::SELF_TYPE: return "SELF_TYPE";
        case CoolLexer::TYPEID: return "TYPEID " + token->getText();
        case CoolLexer::OBJECTID: return "OBJECTID " + token->getText();
        case CoolLexer::STR_CONST: return "STR_CONST ";

        default : return "<Invalid Token>: " + token->toString();
    }

}

void print_hex(ostream &out, char ch) {
    out << "<0x" << std::hex << std::setw(2) << std::setfill('0') << (static_cast<int>(ch) & 0xFF) << ">";
}

void dump_cool_token(CoolLexer *lexer, ostream &out, Token *token) {
    if (token->getType() == static_cast<size_t>(-1)) {
        // Жетонът е EOF, така че не го принтирам.
        return;
    }
    if (token->getType() != CoolLexer::STR_CONST && token->getType() != CoolLexer::ERROR)
        out << "#" << token->getLine() << " " << cool_token_to_string(token);

    auto token_type = token->getType();
    auto token_start_char_index = token->getStartIndex();
    switch (token_type) {
        case CoolLexer::BOOL_CONST:
            out << " "
                << (lexer->get_bool_value(token_start_char_index) ? "true"
                                                                : "false");
            break;
        case CoolLexer::ERROR:{
            auto line = (lexer->errors_end_line[token_start_char_index] != -1)?lexer->errors_end_line[token_start_char_index] :token->getLine();
            out << "#" << line << " ERROR:";
            out  << " " << token->getText() ;
            break;
        }
        case CoolLexer::STR_CONST:
            std::string s = token->getText();
            out << "#" << lexer->str_const_end_line[token_start_char_index] << " STR_CONST ";
                out << '"';
                if (lexer->empty_str.find(token_start_char_index) != lexer->empty_str.end()) {
                    out << '"';
                    break;
                }

                for (size_t i = 0; i < s.size(); ++i) {
                    if (s[i] == '\\') {
                        if (i + 1 < s.length()) {
                            i++;
                            if (s[i] == 'n') {
                                //t_s += "\\n";
                                out << '\\';
                                out << 'n' ;
                            }
                            else if (s[i] == 't') {
                                out << '\\';
                                out << 't';
                            }
                            else if (s[i] == 'b') {
                                out << '\\' << 'b';
                            }
                            else if (s[i] == 'f') {
                                out << '\\' << 'f';
                            }
                            else if (s[i] == '\n') {
                                out << '\\' << '\n';
                            }
                            else {
                                out << '\\' << s[i];
                            }
                        }
                    }
                    else if ((int)s[i] < 32 || (int)s[i] > 126)
                        print_hex(out, s[i]);
                    else out << s[i];
                }
                out << '"';
                break;
    }
                
    // Добавете тук още случаи, за жетони, към които е прикачен специален смисъл.

    //out << endl;
}

int main(int argc, const char *argv[]) {
    ANTLRInputStream input(cin);
    CoolLexer lexer(&input);

    // За временно скриване на грешките:
    //lexer.removeErrorListener(&ConsoleErrorListener::INSTANCE);

    CommonTokenStream tokenStream(&lexer);

    tokenStream.fill(); // Изчитане на всички жетони.

    vector<Token *> tokens = tokenStream.getTokens();
    for (int i = 0; i < tokens.size(); ++i) {
        Token *token = tokens[i];
        dump_cool_token(&lexer, cout, token);
        //if (i != tokens.size() - 1) {
            cout << endl;
        //}
    }

    return 0;
}
