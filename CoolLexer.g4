lexer grammar CoolLexer;
//Authors: Eli (3MI0600110), Stefan (6MI0800588)
// Тази част позволява дефинирането на допълнителен код, който да се запише в
// генерирания CoolLexer.h.
//
// Коментарите вътре са на английски, понеже ANTLR4 иначе ги омазва.
@header {
#include <sstream>
#include <iomanip>
}

@lexer::members {
    // Maximum length of a constant string literal (CSL) excluding the implicit
    // null character that marks the end of the string.
    const unsigned MAX_STR_CONST = 1024;
    // Stores the current CSL as it's being built.
    //std::vector<char> string_buffer;

    // ----------------------- booleans -------------------------

    // A map from token ids to boolean values
    std::map<int, bool> bool_values;

    void assoc_bool_with_token(bool value) {
        bool_values[tokenStartCharIndex] = value;
    }

    bool get_bool_value(int token_start_char_index) {
        return bool_values.at(token_start_char_index);
    }

    // Add your own custom code to be copied to CoolLexer.h here.

    // Variable to store the line number where a block comment starts.
    unsigned int comment_start_line;

    // Custom function to report an unterminated block comment error.
    std::map<int,int> errors_end_line;
    void report_error(const std::string &message, bool change_line = false){
        setText(message);
        setType(ERROR);
        if (change_line)
            errors_end_line[tokenStartCharIndex] = getLine();
        else errors_end_line[tokenStartCharIndex] = -1;
    }

    void report_unmatched_symbol(const std::string& symbol){
        char c = symbol[0];
        std::string message = "Invalid symbol ";
        message.push_back('"');
        if ((int)c < 32 || (int)c > 126){
            std::stringstream ss;
            ss << "<0x" << std::hex << std::setw(2) << std::setfill('0') << (static_cast<int>(c) & 0xFF) << ">";
            message += ss.str();
        }
        else{
            message.push_back(c);
            if (c == '\\')
                message.push_back(c);
        }
        message.push_back('"');
        report_error(message);
    }

    void report_unterminated_string(const std::string& message){
        setText(message);
        setType(ERROR);
        errors_end_line[tokenStartCharIndex] = getLine() - 1;
    }


    std::map<int, int> str_const_end_line;
    std::set<int> empty_str;

    void process_string_constant(const std::string& text) {
        str_const_end_line[tokenStartCharIndex] = getLine();
        std::string str = text;
        str = str.substr(1, str.length() - 2); // Remove the surrounding quotes
        std::string buff;
        size_t size_str = str.length();
        int count_symbols_screen_string = 0;
        for (size_t i = 0; i < size_str; ++i){
            ++count_symbols_screen_string;
            if (str[i] == '\\'){
                assert(i < size_str - 1);
                ++i;
                switch(str[i]){
                    case '\n': buff.push_back('\\');
                               buff.push_back('n');
                               break;
                    case '\t': buff.push_back('\\');
                              buff.push_back('t');
                              break;
                    case '\b': buff.push_back('\\');
                              buff.push_back('b');
                              break;
                    case 'n': buff.push_back('\\');
                              //buff.push_back('\\');
                              buff.push_back('n');
                              break;
                    case '"': buff.push_back('\\');
                              buff.push_back('"');
                              break;
                    case 't': buff.push_back('\\');
                              //buff.push_back('\\');  
                              buff.push_back('t');
                              break;
                    case 'b': buff.push_back('\\');
                              //buff.push_back('\\');  
                              buff.push_back('b');
                              break;
                    case 'f': buff.push_back('\\');
                              //buff.push_back('\\');  
                              buff.push_back('f');
                              break;
                    case '\\': buff.push_back('\\');
                               buff.push_back('\\');
                               break;
                    default: buff.push_back(str[i]);
                             break;
                }
            }
            else if (str[i] == '\t'){
                buff.push_back('\\');
                buff.push_back('t');
            }
            else buff.push_back(str[i]);
        }
        if (count_symbols_screen_string > MAX_STR_CONST)
            report_error("String constant too long",true);
        else{
            if (buff.size() == 0)
                empty_str.insert(tokenStartCharIndex);
            setText(buff);
        }
    }
}



// --------------- прости жетони -------------------
LPAREN : '(';
RPAREN : ')';
LBRACE : '{';
RBRACE : '}';
DOT : '.';
ATYSM: '@';
TILDE: '~';
ISVOID: 'isvoid';
STAR  : '*';
SLASH   : '/';
PLUS  : '+';
MINUS : '-';
LE   : '<=';
LESS  : '<';
EQ    : '=';
ASSIGN: '<-';
COMMA  : ',';
SEMI   : ';';
COLON: ':';
DARROW : '=>';
WS: [ \t\r\n\b\f]+ -> skip ;

// Добавете тук останалите жетони, които представляват просто текст.
INT_CONST: [0-9]+;



// --------------- булеви константи -------------------
BOOL_CONST : 't' ('R'|'r') ('U'|'u') ('E'|'e')  { assoc_bool_with_token(true); }
           | 'f' ('A'|'a') ('L'|'l') ('S'|'s') ('E'|'e') { assoc_bool_with_token(false); };

// --------------- коментари -------------------
LINE_COMMENT : '--' .*? ('\n'|EOF) -> skip;
BLOCK_COMMENT : '(*'   -> pushMode(MULTI_COMMENT), skip;
UNMATCHED_BLOCK_COMMENT_END : '*)' {report_error("Unmatched *)");} ;


// --------------- ключови думи -------------------
CLASS : ('C'|'c') ('L'|'l') ('A'|'a') ('S'|'s') ('S'|'s');
ELSE  : ('E'|'e') ('L'|'l') ('S'|'s') ('E'|'e');
FI    : ('F'|'f') ('I'|'i');
IF    : ('I'|'i') ('F'|'f');
IN    : ('I'|'i') ('N'|'n');
INHERITS : ('I'|'i') ('N'|'n') ('H'|'h') ('E'|'e') ('R'|'r') ('I'|'i') ('T'|'t') ('S'|'s');
LET: ('L'|'l') ('E'|'e') ('T'|'t');
LOOP: ('L'|'l') ('O'|'o') ('O'|'o') ('P'|'p');
POOL: ('P'|'p') ('O'|'o') ('O'|'o') ('L'|'l');
THEN: ('T'|'t') ('H'|'h') ('E'|'e') ('N'|'n');
WHILE: ('W'|'w') ('H'|'h') ('I'|'i') ('L'|'l') ('E'|'e');
CASE: ('C'|'c') ('A'|'a') ('S'|'s') ('E'|'e');
ESAC: ('E'|'e') ('S'|'s') ('A'|'a') ('C'|'c');
NEW: ('N'|'n') ('E'|'e') ('W'|'w');
OF: ('O'|'o') ('F'|'f');
NOT: ('N'|'n') ('O'|'o') ('T'|'t');

SELF: 'self' ;
SELF_TYPE: 'SELF_TYPE' ;

TYPEID: [A-Z][a-zA-Z0-9_]*;
OBJECTID: [a-z][a-zA-Z0-9_]*;




// --------------- текстови низове -------------------
ERROR   : '"' (ESC|~[\n\u0000])*? '\\' '\u0000' (.)*? '"' { report_error("String contains escaped null character", true); }
        | '"' (ESC|~[\n\u0000])*? '\u0000' (.)*? '"' { report_error("String contains null character", true); }
        | '"' (ESC|~["\n\u0000])*? (EOF) { report_error("Unterminated string at EOF", true); }
        | '"' (ESC|~[\n"])*? '\n' {report_unterminated_string("String contains unescaped new line"); };

//UNESCAPED_NEWLINE_STRING: '"' ~["\nEOF]* ('\n') { report_error("String contains unescaped new line"); };


STR_CONST: '"' (ESC|~[\n\u0000])*? '"' {process_string_constant(getText());};
fragment ESC:   '\\\\'| '\\"'| '\\' '\n' ;



// --------------- uncorrect symbols -------------------
NOT_RECOGNIZED: . {report_unmatched_symbol(getText());} ;


mode MULTI_COMMENT;
    BLOCK_COMMENT_END: '*)' -> popMode, skip;
    BLOCK_COMMENT_IN: '(*' -> pushMode(MULTI_COMMENT), skip;
    INCOMMENT : . -> skip;
    ERR : . EOF {report_error("EOF in comment" );} ;
