parser grammar CoolParser;

options { tokenVocab=CoolLexer; }

program: (class ';')+ ;

//class  : CLASS TYPEID '{' (method ';'|attr ';')* '}' ;
class  : CLASS TYPEID (INHERITS TYPEID)? '{' (feature ';')* '}';

feature: OBJECTID '(' (formal(','formal)*)? ')' ':' TYPEID '{' expr '}' #method
        |OBJECTID ':' TYPEID ('<-' expr)? #attr ;

//method : OBJECTID '(' (formal(','formal)*)? ')' ':' TYPEID expr ; // method in class 

//attr : OBJECTID ':' TYPEID ('<-' expr)? ;// attribute in class

formal : OBJECTID ':' TYPEID;

//rewrite expr rule
expr:    BOOL_CONST #bool_const_expr 
        | STR_CONST #str_const_expr
        | INT_CONST #int_const_expr
        | OBJECTID #objectid_expr
        | '(' expr ')' #paren_expr
        | '.' expr #dot_expr
        | '@' expr #at_expr
        | '~' expr #negation_expr     
        | ISVOID expr #isvoid_expr
        | '{' (expr';')+ '}' #block_expr
        | OBJECTID '(' (expr (','expr)*)? ')' #function_call_expr
        | expr '@' TYPEID '.'OBJECTID '(' (expr(','expr)*)? ')' #static_dispatch
        | expr '.' OBJECTID '(' (expr(','expr)*)? ')' #dispatch_expr
        | expr ('*' | '/') expr #multiplicative_expr
        | expr ('+' | '-') expr #additive_expr
        | expr ('<' | '<=' | '=') expr #comparison_expr
        | NOT expr #not_expr
        | NEW TYPEID #new_expr
        | CASE expr OF case_var (case_var)* ESAC #case_expr
        | LET let_var (',' let_var)* IN expr #let_expr
        | WHILE expr LOOP expr POOL #while_expr
        | IF expr THEN expr ELSE expr FI #if_expr
        | OBJECTID '<-'  expr #assign_expr;

let_var : OBJECTID ':' TYPEID ('<-' expr)? ;
case_var : OBJECTID ':' TYPEID '=>' expr ';' ;