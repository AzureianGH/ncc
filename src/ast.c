#include "ast.h"
#include "error_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Create a new AST node
ASTNode* createASTNode(ASTNodeType type) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        fatalError("Out of memory creating AST node");
        return NULL;
    }
    
    memset(node, 0, sizeof(ASTNode));
    node->type = type;
    node->typeInfo = NULL;
    node->line = 0;
    node->column = 0;
    node->filename = NULL;
    node->children = NULL;
    node->childCount = 0;
    
    return node;
}

// Free an AST node and all its children
void freeASTNode(ASTNode* node) {
    if (!node) return;
    
    // Free type information
    if (node->typeInfo) {
        freeTypeInfo(node->typeInfo);
    }
    
    // Free filename string
    if (node->filename) {
        free(node->filename);
    }
    
    // Free node-specific data
    switch (node->type) {
        case AST_IDENTIFIER:
            if (node->data.identifier.name) {
                free(node->data.identifier.name);
            }
            break;
            
        case AST_STRING_LITERAL:
        case AST_WIDE_STRING_LITERAL:
            if (node->data.literal.stringValue) {
                free(node->data.literal.stringValue);
            }
            break;
            
        case AST_BINARY_OP:
            freeASTNode(node->data.binary.left);
            freeASTNode(node->data.binary.right);
            break;
            
        case AST_UNARY_OP:
            freeASTNode(node->data.unary.operand);
            break;
            
        case AST_TERNARY_OP:
            freeASTNode(node->data.ternary.condition);
            freeASTNode(node->data.ternary.trueExpr);
            freeASTNode(node->data.ternary.falseExpr);
            break;
            
        case AST_FUNCTION_CALL:
            freeASTNode(node->data.call.function);
            for (int i = 0; i < node->data.call.argCount; i++) {
                freeASTNode(node->data.call.arguments[i]);
            }
            if (node->data.call.arguments) {
                free(node->data.call.arguments);
            }
            break;
            
        case AST_ARRAY_ACCESS:
            freeASTNode(node->data.arrayAccess.array);
            freeASTNode(node->data.arrayAccess.index);
            break;
            
        case AST_MEMBER_ACCESS:
        case AST_POINTER_ACCESS:
            freeASTNode(node->data.memberAccess.object);
            if (node->data.memberAccess.memberName) {
                free(node->data.memberAccess.memberName);
            }
            break;
            
        case AST_COMPOUND_STMT:
            for (int i = 0; i < node->data.compound.count; i++) {
                freeASTNode(node->data.compound.statements[i]);
            }
            if (node->data.compound.statements) {
                free(node->data.compound.statements);
            }
            break;
            
        case AST_IF_STMT:
            freeASTNode(node->data.ifStmt.condition);
            freeASTNode(node->data.ifStmt.thenStmt);
            freeASTNode(node->data.ifStmt.elseStmt);
            break;
            
        case AST_WHILE_STMT:
        case AST_DO_WHILE_STMT:
            freeASTNode(node->data.whileStmt.condition);
            freeASTNode(node->data.whileStmt.body);
            break;
            
        case AST_FOR_STMT:
            freeASTNode(node->data.forStmt.init);
            freeASTNode(node->data.forStmt.condition);
            freeASTNode(node->data.forStmt.update);
            freeASTNode(node->data.forStmt.body);
            break;
            
        case AST_SWITCH_STMT:
            freeASTNode(node->data.switchStmt.expression);
            for (int i = 0; i < node->data.switchStmt.caseCount; i++) {
                freeASTNode(node->data.switchStmt.cases[i]);
            }
            if (node->data.switchStmt.cases) {
                free(node->data.switchStmt.cases);
            }
            break;
            
        case AST_CASE_STMT:
            freeASTNode(node->data.caseStmt.value);
            freeASTNode(node->data.caseStmt.stmt);
            break;
            
        case AST_RETURN_STMT:
            freeASTNode(node->data.returnStmt.expression);
            break;
            
        case AST_VARIABLE_DECL:
            if (node->data.varDecl.name) {
                free(node->data.varDecl.name);
            }
            if (node->data.varDecl.type) {
                freeTypeInfo(node->data.varDecl.type);
            }
            freeASTNode(node->data.varDecl.initializer);
            break;
            
        case AST_FUNCTION_DECL:
            if (node->data.funcDecl.name) {
                free(node->data.funcDecl.name);
            }
            if (node->data.funcDecl.returnType) {
                freeTypeInfo(node->data.funcDecl.returnType);
            }
            for (int i = 0; i < node->data.funcDecl.paramCount; i++) {
                if (node->data.funcDecl.parameters[i]) {
                    if (node->data.funcDecl.parameters[i]->name) {
                        free(node->data.funcDecl.parameters[i]->name);
                    }
                    if (node->data.funcDecl.parameters[i]->type) {
                        freeTypeInfo(node->data.funcDecl.parameters[i]->type);
                    }
                    free(node->data.funcDecl.parameters[i]);
                }
            }
            if (node->data.funcDecl.parameters) {
                free(node->data.funcDecl.parameters);
            }
            freeASTNode(node->data.funcDecl.body);
            break;
            
        case AST_INLINE_ASM:
            if (node->data.inlineAsm.assembly) {
                free(node->data.inlineAsm.assembly);
            }
            for (int i = 0; i < node->data.inlineAsm.outputCount; i++) {
                if (node->data.inlineAsm.outputConstraints[i]) {
                    free(node->data.inlineAsm.outputConstraints[i]);
                }
                if (node->data.inlineAsm.outputOperands && node->data.inlineAsm.outputOperands[i]) {
                    free(node->data.inlineAsm.outputOperands[i]);
                }
            }
            if (node->data.inlineAsm.outputConstraints) {
                free(node->data.inlineAsm.outputConstraints);
            }
            if (node->data.inlineAsm.outputOperands) {
                free(node->data.inlineAsm.outputOperands);
            }
            for (int i = 0; i < node->data.inlineAsm.inputCount; i++) {
                if (node->data.inlineAsm.inputConstraints[i]) {
                    free(node->data.inlineAsm.inputConstraints[i]);
                }
                if (node->data.inlineAsm.inputOperands && node->data.inlineAsm.inputOperands[i]) {
                    free(node->data.inlineAsm.inputOperands[i]);
                }
            }
            if (node->data.inlineAsm.inputConstraints) {
                free(node->data.inlineAsm.inputConstraints);
            }
            if (node->data.inlineAsm.inputOperands) {
                free(node->data.inlineAsm.inputOperands);
            }
            for (int i = 0; i < node->data.inlineAsm.clobberCount; i++) {
                if (node->data.inlineAsm.clobbers[i]) {
                    free(node->data.inlineAsm.clobbers[i]);
                }
            }
            if (node->data.inlineAsm.clobbers) {
                free(node->data.inlineAsm.clobbers);
            }
            break;
            
        case AST_INITIALIZER_LIST:
            for (int i = 0; i < node->data.initList.count; i++) {
                freeASTNode(node->data.initList.elements[i]);
            }
            if (node->data.initList.elements) {
                free(node->data.initList.elements);
            }
            break;
            
        case AST_DESIGNATED_INITIALIZER:
            for (int i = 0; i < node->data.designatedInit.designatorCount; i++) {
                freeASTNode(node->data.designatedInit.designators[i]);
            }
            if (node->data.designatedInit.designators) {
                free(node->data.designatedInit.designators);
            }
            freeASTNode(node->data.designatedInit.value);
            break;
            
        default:
            // Many other node types would need handling here
            break;
    }
    
    // Free children array
    if (node->children) {
        free(node->children);
    }
    
    free(node);
}

// Copy an AST node (deep copy)
ASTNode* copyASTNode(ASTNode* node) {
    if (!node) return NULL;
    
    ASTNode* copy = createASTNode(node->type);
    if (!copy) return NULL;
    
    copy->typeInfo = node->typeInfo ? copyTypeInfo(node->typeInfo) : NULL;
    copy->line = node->line;
    copy->column = node->column;
    copy->filename = node->filename ? strdup(node->filename) : NULL;
    
    // Copy node-specific data - this would be extensive for all node types
    // For now, just copy the basic structure
    copy->data = node->data;
    
    return copy;
}

// Type system functions
TypeInfo* createTypeInfo(DataType baseType) {
    TypeInfo* type = malloc(sizeof(TypeInfo));
    if (!type) {
        fatalError("Out of memory creating type info");
        return NULL;
    }
    
    memset(type, 0, sizeof(TypeInfo));
    type->baseType = baseType;
    type->qualifiers = QUAL_NONE;
    type->storageClass = STORAGE_NONE;
    type->funcSpecifiers = FUNC_NONE;
    type->size = 0;
    type->alignment = 0;
    
    return type;
}

void freeTypeInfo(TypeInfo* type) {
    if (!type) return;
    
    if (type->elementType) {
        freeTypeInfo(type->elementType);
    }
    
    if (type->pointsTo) {
        freeTypeInfo(type->pointsTo);
    }
    
    if (type->returnType) {
        freeTypeInfo(type->returnType);
    }
    
    if (type->paramTypes) {
        for (int i = 0; i < type->paramCount; i++) {
            freeTypeInfo(type->paramTypes[i]);
        }
        free(type->paramTypes);
    }
    
    if (type->structName) {
        free(type->structName);
    }
    
    if (type->enumName) {
        free(type->enumName);
    }
    
    free(type);
}

TypeInfo* copyTypeInfo(TypeInfo* type) {
    if (!type) return NULL;
    
    TypeInfo* copy = createTypeInfo(type->baseType);
    if (!copy) return NULL;
    
    copy->qualifiers = type->qualifiers;
    copy->storageClass = type->storageClass;
    copy->funcSpecifiers = type->funcSpecifiers;
    copy->arraySize = type->arraySize;
    copy->paramCount = type->paramCount;
    copy->isVariadic = type->isVariadic;
    copy->memberCount = type->memberCount;
    copy->enumCount = type->enumCount;
    copy->size = type->size;
    copy->alignment = type->alignment;
    
    copy->elementType = type->elementType ? copyTypeInfo(type->elementType) : NULL;
    copy->pointsTo = type->pointsTo ? copyTypeInfo(type->pointsTo) : NULL;
    copy->returnType = type->returnType ? copyTypeInfo(type->returnType) : NULL;
    
    copy->structName = type->structName ? strdup(type->structName) : NULL;
    copy->enumName = type->enumName ? strdup(type->enumName) : NULL;
    
    // Copy parameter types
    if (type->paramTypes && type->paramCount > 0) {
        copy->paramTypes = malloc(sizeof(TypeInfo*) * type->paramCount);
        for (int i = 0; i < type->paramCount; i++) {
            copy->paramTypes[i] = copyTypeInfo(type->paramTypes[i]);
        }
    }
    
    return copy;
}

// Type size calculation
int getTypeSize(TypeInfo* type, int targetWidth) {
    if (!type) return 0;
    
    switch (type->baseType) {
        case TYPE_VOID:
            return 1; // void* is pointer-sized
        case TYPE_CHAR:
            return 1;
        case TYPE_SHORT:
            return 2;
        case TYPE_INT:
            return targetWidth <= 16 ? 2 : 4;
        case TYPE_LONG:
            return targetWidth == 64 ? 8 : 4;
        case TYPE_LONG_LONG:
            return 8;
        case TYPE_FLOAT:
            return 4;
        case TYPE_DOUBLE:
            return 8;
        case TYPE_LONG_DOUBLE:
            return targetWidth == 64 ? 16 : 10;
        case TYPE_BOOL:
            return 1;
        case TYPE_POINTER:
            return targetWidth / 8;
        case TYPE_ARRAY:
            if (type->elementType) {
                return type->arraySize * getTypeSize(type->elementType, targetWidth);
            }
            return 0;
        case TYPE_STRUCT:
        case TYPE_UNION:
            // Would need to calculate based on members
            return type->size;
        default:
            return 0;
    }
}

int getTypeAlignment(TypeInfo* type, int targetWidth) {
    if (!type) return 1;
    
    switch (type->baseType) {
        case TYPE_CHAR:
        case TYPE_BOOL:
            return 1;
        case TYPE_SHORT:
            return 2;
        case TYPE_INT:
        case TYPE_FLOAT:
            return targetWidth <= 16 ? 2 : 4;
        case TYPE_LONG:
        case TYPE_DOUBLE:
            return targetWidth == 64 ? 8 : 4;
        case TYPE_LONG_LONG:
            return 8;
        case TYPE_LONG_DOUBLE:
            return targetWidth == 64 ? 16 : 4;
        case TYPE_POINTER:
            return targetWidth / 8;
        case TYPE_ARRAY:
            return type->elementType ? getTypeAlignment(type->elementType, targetWidth) : 1;
        case TYPE_STRUCT:
        case TYPE_UNION:
            return type->alignment;
        default:
            return 1;
    }
}

// Type compatibility checking
int areTypesCompatible(TypeInfo* type1, TypeInfo* type2) {
    if (!type1 || !type2) return 0;
    
    if (type1->baseType != type2->baseType) return 0;
    
    // Check qualifiers (const, volatile, restrict)
    if (type1->qualifiers != type2->qualifiers) return 0;
    
    switch (type1->baseType) {
        case TYPE_POINTER:
            return areTypesCompatible(type1->pointsTo, type2->pointsTo);
        case TYPE_ARRAY:
            if (type1->arraySize != type2->arraySize) return 0;
            return areTypesCompatible(type1->elementType, type2->elementType);
        case TYPE_FUNCTION:
            if (!areTypesCompatible(type1->returnType, type2->returnType)) return 0;
            if (type1->paramCount != type2->paramCount) return 0;
            if (type1->isVariadic != type2->isVariadic) return 0;
            for (int i = 0; i < type1->paramCount; i++) {
                if (!areTypesCompatible(type1->paramTypes[i], type2->paramTypes[i])) {
                    return 0;
                }
            }
            return 1;
        default:
            return 1;
    }
}

// Get common type for arithmetic operations
TypeInfo* getCommonType(TypeInfo* type1, TypeInfo* type2) {
    if (!type1 || !type2) return NULL;
    
    // Simplified type promotion rules
    if (type1->baseType == TYPE_DOUBLE || type2->baseType == TYPE_DOUBLE) {
        return createTypeInfo(TYPE_DOUBLE);
    }
    if (type1->baseType == TYPE_FLOAT || type2->baseType == TYPE_FLOAT) {
        return createTypeInfo(TYPE_FLOAT);
    }
    if (type1->baseType == TYPE_LONG_LONG || type2->baseType == TYPE_LONG_LONG) {
        return createTypeInfo(TYPE_LONG_LONG);
    }
    if (type1->baseType == TYPE_LONG || type2->baseType == TYPE_LONG) {
        return createTypeInfo(TYPE_LONG);
    }
    
    return createTypeInfo(TYPE_INT);
}

// C99 type checking functions
int isIntegerType(TypeInfo* type) {
    if (!type) return 0;
    return type->baseType == TYPE_CHAR || type->baseType == TYPE_SHORT ||
           type->baseType == TYPE_INT || type->baseType == TYPE_LONG ||
           type->baseType == TYPE_LONG_LONG || type->baseType == TYPE_BOOL;
}

int isFloatingType(TypeInfo* type) {
    if (!type) return 0;
    return type->baseType == TYPE_FLOAT || type->baseType == TYPE_DOUBLE ||
           type->baseType == TYPE_LONG_DOUBLE;
}

int isArithmeticType(TypeInfo* type) {
    return isIntegerType(type) || isFloatingType(type);
}

int isScalarType(TypeInfo* type) {
    return isArithmeticType(type) || type->baseType == TYPE_POINTER;
}

int isComplexType(TypeInfo* type) {
    if (!type) return 0;
    return type->baseType == TYPE_COMPLEX_FLOAT ||
           type->baseType == TYPE_COMPLEX_DOUBLE ||
           type->baseType == TYPE_COMPLEX_LONG_DOUBLE;
}

int isRealType(TypeInfo* type) {
    return isArithmeticType(type) && !isComplexType(type);
}

// AST utility functions
void addChild(ASTNode* parent, ASTNode* child) {
    if (!parent || !child) return;
    
    parent->children = realloc(parent->children, sizeof(ASTNode*) * (parent->childCount + 1));
    if (!parent->children) {
        fatalError("Out of memory adding AST child");
        return;
    }
    
    parent->children[parent->childCount] = child;
    parent->childCount++;
}

ASTNode* findNodeByType(ASTNode* root, ASTNodeType type) {
    if (!root) return NULL;
    
    if (root->type == type) return root;
    
    for (int i = 0; i < root->childCount; i++) {
        ASTNode* result = findNodeByType(root->children[i], type);
        if (result) return result;
    }
    
    return NULL;
}

void visitAST(ASTNode* node, void (*visitor)(ASTNode*, void*), void* context) {
    if (!node || !visitor) return;
    
    visitor(node, context);
    
    for (int i = 0; i < node->childCount; i++) {
        visitAST(node->children[i], visitor, context);
    }
}

// Print AST for debugging
static void printIndent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

static const char* getNodeTypeName(ASTNodeType type) {
    switch (type) {
        case AST_PROGRAM: return "PROGRAM";
        case AST_TRANSLATION_UNIT: return "TRANSLATION_UNIT";
        case AST_FUNCTION_DECL: return "FUNCTION_DECL";
        case AST_VARIABLE_DECL: return "VARIABLE_DECL";
        case AST_IDENTIFIER: return "IDENTIFIER";
        case AST_INTEGER_LITERAL: return "INTEGER_LITERAL";
        case AST_FLOAT_LITERAL: return "FLOAT_LITERAL";
        case AST_STRING_LITERAL: return "STRING_LITERAL";
        case AST_CHAR_LITERAL: return "CHAR_LITERAL";
        case AST_BINARY_OP: return "BINARY_OP";
        case AST_UNARY_OP: return "UNARY_OP";
        case AST_FUNCTION_CALL: return "FUNCTION_CALL";
        case AST_IF_STMT: return "IF_STMT";
        case AST_WHILE_STMT: return "WHILE_STMT";
        case AST_FOR_STMT: return "FOR_STMT";
        case AST_RETURN_STMT: return "RETURN_STMT";
        case AST_COMPOUND_STMT: return "COMPOUND_STMT";
        case AST_INLINE_ASM: return "INLINE_ASM";
        default: return "UNKNOWN";
    }
}

void printAST(ASTNode* node, int indent) {
    if (!node) return;
    
    printIndent(indent);
    printf("%s", getNodeTypeName(node->type));
    
    switch (node->type) {
        case AST_IDENTIFIER:
            if (node->data.identifier.name) {
                printf(" (%s)", node->data.identifier.name);
            }
            break;
        case AST_INTEGER_LITERAL:
            printf(" (%lld)", node->data.literal.intValue);
            break;
        case AST_FLOAT_LITERAL:
            printf(" (%f)", node->data.literal.floatValue);
            break;
        case AST_STRING_LITERAL:
            if (node->data.literal.stringValue) {
                printf(" (\"%s\")", node->data.literal.stringValue);
            }
            break;
        case AST_FUNCTION_DECL:
        case AST_VARIABLE_DECL:
            if (node->data.varDecl.name) {
                printf(" (%s)", node->data.varDecl.name);
            }
            break;
        default:
            break;
    }
    
    printf("\n");
    
    // Print type-specific children
    switch (node->type) {
        case AST_BINARY_OP:
            printAST(node->data.binary.left, indent + 1);
            printAST(node->data.binary.right, indent + 1);
            break;
        case AST_UNARY_OP:
            printAST(node->data.unary.operand, indent + 1);
            break;
        case AST_FUNCTION_CALL:
            printAST(node->data.call.function, indent + 1);
            for (int i = 0; i < node->data.call.argCount; i++) {
                printAST(node->data.call.arguments[i], indent + 1);
            }
            break;
        case AST_IF_STMT:
            printAST(node->data.ifStmt.condition, indent + 1);
            printAST(node->data.ifStmt.thenStmt, indent + 1);
            if (node->data.ifStmt.elseStmt) {
                printAST(node->data.ifStmt.elseStmt, indent + 1);
            }
            break;
        case AST_COMPOUND_STMT:
            for (int i = 0; i < node->data.compound.count; i++) {
                printAST(node->data.compound.statements[i], indent + 1);
            }
            break;
        case AST_FUNCTION_DECL:
            if (node->data.funcDecl.body) {
                printAST(node->data.funcDecl.body, indent + 1);
            }
            break;
        case AST_VARIABLE_DECL:
            if (node->data.varDecl.initializer) {
                printAST(node->data.varDecl.initializer, indent + 1);
            }
            break;
        default:
            // Print generic children
            for (int i = 0; i < node->childCount; i++) {
                printAST(node->children[i], indent + 1);
            }
            break;
    }
}

// AST cleanup
void cleanupAST(ASTNode* root) {
    freeASTNode(root);
}
