/**
 * @file  syntax.h
 *
 * @brief Defines MACROS for reserved syntax keywords and anotations for the parser
 *
 * @project IFJ26 compiler
 * @author Matúš Čík
 */

#ifndef SYNTAX_H
#define SYNTAX_H

#define SYNTAX_KW_IMPORT "import"
#define SYNTAX_KW_DEF "def"
#define SYNTAX_KW_IF "if"
#define SYNTAX_KW_THEN "then"
#define SYNTAX_KW_ELSE "else"
#define SYNTAX_KW_WHILE "while"
#define SYNTAX_KW_FOR "for"
#define SYNTAX_KW_TO "to"
#define SYNTAX_KW_UNTIL "until"
#define SYNTAX_KW_BY "by"
#define SYNTAX_KW_DO "do"
#define SYNTAX_KW_RETURN "return"
#define SYNTAX_KW_VAL "val"
#define SYNTAX_KW_VAR "var"

#define SYNTAX_KW_INT "Int"
#define SYNTAX_KW_DOUBLE "Double"
#define SYNTAX_KW_STRING "String"
#define SYNTAX_KW_UNIT "Unit"
#define SYNTAX_KW_BOOLEAN "Boolean"
#define SYNTAX_KW_LIST "List"
#define SYNTAX_KW_TYPE "type"

#define SYNTAX_PROLOG_NAME "ifj26"
#define SYNTAX_ANNOTATION_MAIN "main"
#define SYNTAX_WILDCARD "*"

#define SYNTAX_OP_LEFT_PAREN "("
#define SYNTAX_OP_ASSIGN "="
#define SYNTAX_OP_GENERATOR_ARROW "<-"
#define SYNTAX_OP_PLUS "+"
#define SYNTAX_OP_MINUS "-"
#define SYNTAX_OP_MULTIPLY "*"
#define SYNTAX_OP_DIVIDE "/"
#define SYNTAX_OP_LESS "<"
#define SYNTAX_OP_LESS_EQUAL "<="
#define SYNTAX_OP_GREATER ">"
#define SYNTAX_OP_GREATER_EQUAL ">="
#define SYNTAX_OP_EQUAL "=="
#define SYNTAX_OP_NOT_EQUAL "!="
#define SYNTAX_OP_LOGICAL_NOT "!"
#define SYNTAX_OP_LOGICAL_AND "&&"
#define SYNTAX_OP_LOGICAL_OR "||"

#endif
