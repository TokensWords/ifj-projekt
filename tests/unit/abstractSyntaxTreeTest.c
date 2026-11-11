/*
 * Project: IFJ26 compiler
 * Author: AI testing agent.
 * Prompt: Test tree defaults, owned text, children, and destruction.
 */

#include "abstractSyntaxTree.h"

#include <assert.h>
#include <string.h>

int main(void) {
    SourceLocation rootLocation = {.line = 3, .column = 5};
    AbstractSyntaxNode *root = abstractSyntaxNodeCreate(ABSTRACT_SYNTAX_PROGRAM, rootLocation);
    assert(root != NULL);
    assert(root->kind == ABSTRACT_SYNTAX_PROGRAM);
    assert(root->location.line == 3);
    assert(root->location.column == 5);
    assert(root->inferredType == NULL);
    assert(root->variableType == NULL);
    assert(!root->isMutable);
    assert(!root->isEntryFunction);
    assert(root->text == NULL);
    assert(root->children == NULL);
    assert(root->childCount == 0);
    assert(root->childCapacity == 0);

    char sourceText[] = "program";
    assert(abstractSyntaxNodeSetText(root, sourceText) == COMPILER_OK);
    assert(root->text != sourceText);
    assert(strcmp(root->text, "program") == 0);
    sourceText[0] = 'P';
    assert(strcmp(root->text, "program") == 0);

    for (size_t index = 0; index < 5; index++) {
        SourceLocation childLocation = {.line = index + 4, .column = 1};
        AbstractSyntaxNode *child =
            abstractSyntaxNodeCreate(ABSTRACT_SYNTAX_IDENTIFIER, childLocation);
        assert(child != NULL);
        assert(abstractSyntaxNodeSetText(child, "value") == COMPILER_OK);
        assert(abstractSyntaxNodeAddChild(root, child) == COMPILER_OK);
        assert(root->children[index] == child);
    }

    assert(root->childCount == 5);
    assert(root->childCapacity >= 5);
    assert(abstractSyntaxNodeAddChild(NULL, root) == COMPILER_INTERNAL_ERROR);
    assert(abstractSyntaxNodeAddChild(root, NULL) == COMPILER_INTERNAL_ERROR);
    assert(abstractSyntaxNodeSetText(NULL, "invalid") == COMPILER_INTERNAL_ERROR);
    assert(abstractSyntaxNodeSetText(root, NULL) == COMPILER_INTERNAL_ERROR);

    SourceProgram *program = sourceProgramCreate();
    assert(program != NULL);
    assert(program->root == NULL);
    program->root = root;
    sourceProgramDestroy(program);
    sourceProgramDestroy(NULL);
    abstractSyntaxNodeDestroy(NULL);
    return 0;
}
