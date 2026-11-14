/*
 * Project: IFJ26 compiler
 * Authors: Václav Krytinář xkrytiv00
 * AI-assisted scaffold.
 * Prompt: Define semantic-analysis inputs, outputs, and status reporting.
 */

#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "abstractSyntaxTree.h"
#include "compilerTypes.h"
#include "symbolTable.h"
#include "typeDescriptor.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct SemAnalData SemAnalData;

/**
 * @brief Creates a type descriptor with the specified datatype.
 * @param type The datatype to initialize the descriptor with.
 * @return The created type descriptor, or NULL on allocation failure.
 */
TypeDescriptor *semAnalInitTypeDescriptor(DataType type);

/**
 * @brief Creates a deep copy of a type descriptor.
 * @param src The descriptor to copy.
 * @return The copied descriptor, or NULL if src is NULL or allocation fails.
 */
TypeDescriptor *semAnalCopyTypeDescriptor(const TypeDescriptor *src);

/**
 * @brief Frees a type descriptor and its contents.
 * @param desc The descriptor to destroy.
 */
void semAnalDestroyTypeDescriptor(TypeDescriptor *desc);

/**
 * @brief Resolves aliases and case-class names in a type descriptor.
 * @param desc The descriptor to normalize.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalNormalizeTypeDescriptor(TypeDescriptor *desc, SemAnalData *data);

/**
 * @brief Checks whether one type descriptor is a subtype of another.
 * @param superType The type that accepts values of subType.
 * @param subType The type to check.
 * @return true if subType is a subtype of superType, otherwise false.
 */
bool semAnalIsSubtype(TypeDescriptor *superType, TypeDescriptor *subType);

/**
 * @brief Creates a type descriptor representing the union of two types.
 * @param typeA The first type descriptor.
 * @param typeB The second type descriptor.
 * @return The unified descriptor, or NULL on allocation failure.
 */
TypeDescriptor *semAnalUnifyTypes(TypeDescriptor *typeA, TypeDescriptor *typeB);

/**
 * @brief Initializes semantic-analysis data and creates its function symbol table.
 * @param data The semantic-analysis data to initialize.
 * @return status of the program - error codes
 */
CompilerStatus semAnalDataInit(SemAnalData *data);

/**
 * @brief Expands the semantic-analysis symbol-table and recursion lists.
 * @param data The semantic-analysis data containing the lists.
 * @return status of the program - error codes
 */
CompilerStatus semAnalListsBigger(SemAnalData *data);

/**
 * @brief Removes the last entries from the semantic-analysis lists.
 * @param data The semantic-analysis data containing the lists.
 * @return status of the program - error codes
 */
CompilerStatus semAnalListsSmaller(SemAnalData *data);

/**
 * @brief Destroys resources owned by semantic-analysis data.
 * @param data The semantic-analysis data to destroy.
 */
void semAnalDataDestroy(SemAnalData *data);

/**
 * @brief goes through functions in AST and checks semantics
 * @param program sorce program AST got from syntax analyser
 * @return status of the program - error codes
 */
CompilerStatus semanticAnalyzerAnalyze(SourceProgram *program);

/**
 * @brief Allocates a new symbol and initializes with default values.
 * @return The created symbol.
 */
Symbol *semAnalCreateSymbolDefault();

/**
 * @brief Frees a symbol and its allocated function parameter types.
 * @param symbol The symbol to free.
 */
void semAnalFreeSymbol(Symbol *symbol);

/**
 * @brief gets parameter types from AST node
 * @param node pointer to AST function node to get parameter types from
 * @return An array of copied parameter type descriptors.
 */
TypeDescriptor **semAnalAstGetParameterTypes(AbstractSyntaxNode *node);

/**
 * @brief Gets parameter names from an AST function or case-class node.
 * @param node The AST node to get parameter names from.
 * @return An array of parameter names, or NULL if there are no parameters or node is invalid.
 */
char **semAnalAstGetParameterNames(AbstractSyntaxNode *node);

/**
 * @brief Extracts information from current ast node to a new symbol.
 * This function does only extract information from the node + function parameter types from
 * function nodes. It does NOT set coresponding scope depth (default 0). It sets defined to false.
 * @param node node to be extracted from
 * @return The created symbol, or NULL if the node cannot be converted.
 */
Symbol *semAnalAstNodeToSymbol(AbstractSyntaxNode *node);

/**
 * @brief Adds builtin functions to the table of symbols.
 * @param symbols Table of symbols to insert builtin functions to.
 * @return status of the program - error codes
 */
CompilerStatus semAnalDefineBuiltinFunctions(SymbolTable *symbols);

/**
 * @brief Resolves a global algebraic data type declaration.
 * @param algebdtNode The algebraic data type AST node to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveGlobalAlgebdt(AbstractSyntaxNode *algebdtNode, SemAnalData *data);

/**
 * @brief Adds all user functions to the table of symbols.
 * It adds function name, explicit return type and parameters.
 * It does not check the function body or correct return semantics.
 * It also calls a function to resolve definitions of types and classes if it encounters them.
 * @param rootNode Root node of the AST.
 * @param data The semantic-analysis data to use. Contains the functions symbol table.
 * @return status of the program - error codes
 */
CompilerStatus semAnalDeclareFunctions(AbstractSyntaxNode *rootNode, SemAnalData *data);

/**
 * @brief Checks that an entry function has no parameters and returns unit.
 * @param functionNode The function AST node to check.
 * @return status of the program - error codes
 */
CompilerStatus semAnalCheckMain(AbstractSyntaxNode *functionNode);

/**
 * @brief Checks whether an AST node ends with a return statement.
 * @param node The AST node to check.
 * @return true if the node ends with a return statement, otherwise false.
 */
bool semAnalNodeEndsWithReturn(AbstractSyntaxNode *node);

/**
 * @brief Checks whether an if node can be an expression.
 * @param ifNode The if-statement AST node to check.
 * @return true if the if node meets the rules, otherwise false.
 */
bool semAnalIsIfExpression(AbstractSyntaxNode *ifNode);

/**
 * @brief Checks whether both branches of an if statement return.
 * @param ifNode The if-statement AST node to check.
 * @return true if all branches return, otherwise false.
 */
bool semAnalCheckIfReturns(AbstractSyntaxNode *ifNode);

/**
 * @brief Resolves and validates a function's return type from its body.
 * @param functionNode The function AST node.
 * @param functionSymbol The symbol associated with the function.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveFunctionReturnType(AbstractSyntaxNode *functionNode, Symbol *functionSymbol);

/**
 * @brief Defines a function by resolving its parameters and body.
 * @param functionNode The function AST node to define.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalDefineFunction(AbstractSyntaxNode *functionNode, SemAnalData *data);

/**
 * @brief Declares a function parameter in the symbol table.
 * @param parameterNode The parameter AST node to declare.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalDeclareParameter(AbstractSyntaxNode *parameterNode, SemAnalData *data);

/**
 * @brief Resolves the statements in a block.
 * @param blockNode The block AST node to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveBlock(AbstractSyntaxNode *blockNode, SemAnalData *data);

/**
 * @brief Resolves an AST node and performs semantic checks.
 * Basically a switch to call correct resolve function.
 * @param node The AST node to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveNode(AbstractSyntaxNode *node, SemAnalData *data);

/**
 * @brief Resolves a case-class declaration and its constructor symbol.
 * @param classNode The case-class AST node to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolvecCaseClass(AbstractSyntaxNode *classNode, SemAnalData *data);

/**
 * @brief Resolves an if statement and its branches.
 * @param ifNode The if-statement AST node to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveIf(AbstractSyntaxNode *ifNode, SemAnalData *data);

/**
 * @brief Resolves a for loop, including its generator, bounds, guards, and body.
 * @param forNode The for-loop AST node to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveFor(AbstractSyntaxNode *forNode, SemAnalData *data);

/**
 * @brief Resolves a while statement and its body.
 * @param whileNode The while-statement AST node to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveWhile(AbstractSyntaxNode *whileNode, SemAnalData *data);

/**
 * @brief Resolves a return statement and checks its return type.
 * @param returnNode The return-statement AST node to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveReturn(AbstractSyntaxNode *returnNode, SemAnalData *data);

/**
 * @brief Resolves a for-loop generator clause and declares its binding.
 * @param clause The generator clause to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveForGenerator(AbstractSyntaxNode *clause, SemAnalData *data);

/**
 * @brief Resolves and validates a for-loop guard clause.
 * @param clause The guard clause to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveForGuard(AbstractSyntaxNode *clause, SemAnalData *data);

/**
 * @brief Resolves a member access expression and determines its type.
 * @param accessNode The member-access AST node to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveMemberAccess(AbstractSyntaxNode *accessNode, SemAnalData *data);

/**
 * @brief Resolves a case-class pattern and validates its bindings.
 * @param patternNode The case-class pattern AST node to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveCaseClassPattern(AbstractSyntaxNode *patternNode, SemAnalData *data);

/**
 * @brief Resolves a match-pattern node against its scrutinee.
 * @param patternNode The pattern AST node to resolve.
 * @param scrutinee The expression whose value is being matched.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveMatchPattern(AbstractSyntaxNode *patternNode, AbstractSyntaxNode *scrutinee, SemAnalData *data);

/**
 * @brief Resolves a match expression and its branches.
 * @param matchNode The match-expression AST node to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveMatchExpression(AbstractSyntaxNode *matchNode, SemAnalData *data);

/**
 * @brief Finds a function in the current recursion list.
 * @param funName The function name to find.
 * @param data The semantic-analysis data containing the recursion list.
 * @return The function's index if found, -1 if it is not recursive, or -2 on invalid input.
 */
int semAnalInRecursion(const char *funName, SemAnalData *data);

/**
 * @brief Checks that functions in the recursion list have declared return types.
 * @param recursionI The index in the recursion list to start checking from.
 * @param data The semantic-analysis data containing the recursion list and function symbols.
 * @return status of the program - error codes
 */
CompilerStatus semAnalCheckRecursion(size_t recursionI, SemAnalData *data);

/**
 * @brief Resolves a list-constructor call and infers its element type.
 * @param functionCallNode The list-constructor call AST node.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveListConstructor(AbstractSyntaxNode *functionCallNode, SemAnalData *data);

/**
 * @brief Resolves a regular function call and checks its arguments.
 * @param functionCallNode The function-call AST node.
 * @param data The semantic-analysis data to use.
 * @param callee The function identifier AST node.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveStandardCall(AbstractSyntaxNode *functionCallNode, SemAnalData *data, AbstractSyntaxNode *callee);

/**
 * @brief Resolves a method call on a list value.
 * @param callNode The method-call AST node.
 * @param data The semantic-analysis data to use.
 * @param receiver The list expression receiving the method call.
 * @param methodName The name of the list method.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveListMethodCall(AbstractSyntaxNode *callNode, SemAnalData *data, AbstractSyntaxNode *receiver, const char *methodName);

/**
 * @brief Resolves a function call and checks its arguments.
 * @param functionCallNode The function-call AST node to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveFunctionCall(AbstractSyntaxNode *functionCallNode, SemAnalData *data);

/**
 * @brief Resolves a variable declaration and checks its type.
 * @param variableDeclarationNode The variable-declaration AST node to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveVariableDeclaration(AbstractSyntaxNode *variableDeclarationNode, SemAnalData *data);

/**
 * @brief Resolves an assignment and checks its type.
 * @param assignmentNode The assignment AST node to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveAssignment(AbstractSyntaxNode *assignmentNode, SemAnalData *data);

/**
 * @brief Resolves an expression statement.
 * @param expressionStatementNode The expression-statement AST node to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveExpressionStatement(AbstractSyntaxNode *expressionStatementNode, SemAnalData *data);

/**
 * @brief Resolves a binary expression and checks operand types.
 * @param expressionBinaryNode The binary-expression AST node to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveExpressionBinary(AbstractSyntaxNode *expressionBinaryNode, SemAnalData *data);

/**
 * @brief Resolves a unary expression and checks its operand type.
 * @param unaryExpressionNode The unary-expression AST node to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveUnaryExpression(AbstractSyntaxNode *unaryExpressionNode, SemAnalData *data);

/**
 * @brief Resolves an identifier and looks up its symbol.
 * @param identifierNode The identifier AST node to resolve.
 * @param data The semantic-analysis data to use.
 * @return status of the program - error codes
 */
CompilerStatus semAnalResolveIdentifier(AbstractSyntaxNode *identifierNode, SemAnalData *data);

/**
 * @brief Gets the type descriptor produced by a "line of code".
 * @param node The AST node to evaluate.
 * @return The resulting type descriptor, or NULL if it cannot be determined.
 */
TypeDescriptor *semAnalEvaluatesToType(AbstractSyntaxNode *node);

#endif
