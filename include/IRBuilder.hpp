#pragma once

#include "IR.hpp"
#include <map>
#include <memory>
#include <string>

// --- I. Global Setup (External Declarations) ---
// These are defined in IRBuilder.cpp
extern Tensor TensorA;
extern Tensor TensorB;
extern Tensor TensorC;
extern std::map<std::string, Tensor *> TensorMap;

// --- II. Public Interface for the Builder ---

/**
 * @brief Parses an input program string into a complete IR tree.
 * @param input_program The source code string (LOOPS:..., BODY:...).
 * @return A unique_ptr to the root IRNode (usually a Loop).
 */
std::unique_ptr<IRNode> buildUntiledIR(const std::string &input_program);

// --- III. Verification/Debugging Interface ---

/**
 * @brief Traverses and prints the IR tree structure.
 * @param node The root node to start printing from.
 * @param depth The current indentation level.
 */
void printIR(const IRNode *node, int depth = 0);