#pragma once

#include "IR.hpp"
#include <iostream>
#include <string>

/**
 * @brief Recursively generates C++ code from the IR tree into an output stream.
 *
 * @param root A pointer to the root of the IRNode subtree to generate code for.
 * @param depth The current indentation level.
 * @param os The output stream to write the generated C++ code to.
 */
void codeGeneration(const IRNode *root, int depth, std::ostream &os);

/**
 * @brief Recursively generates the C++ code for an expression (Const, Variable,
 * Add, Mul, Min, Load).
 *
 * @param node A pointer to the root of the expression IRNode.
 * @return A string containing the C++ representation of the expression.
 */
std::string generateExpression(const IRNode *node);

/**
 * @brief Top-level function to generate C++ code into files/console for
 * benchmarking.
 *
 * @param untiled_root The root of the original, untiled IR.
 * @param tiled_root The root of the transformed, tiled IR.
 * @param kernel_type The name/type of the kernel (e.g., "add", "transpose").
 */
void generateCodeFiles(const IRNode *untiled_root, const IRNode *tiled_root,
                       const std::string &kernel_type);