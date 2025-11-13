#include "IRBuilder.hpp"
#include <iostream>

int main() {

  // --- TEST 1: Matrix Multiplication Core (3D Loop Nest, Precedence) ---
  const std::string mm_program = R"(
        LOOPS: i=0:N:1, j=0:M:1, k=0:K:1
        BODY: C[i, j] = C[i, j] + (A[i, k] * B[k, j])
    )";

  std::cout << "--- TEST 1: Matrix Multiplication (3D, Precedence) ---"
            << std::endl;
  try {
    std::unique_ptr<IRNode> mm_ir_root = buildUntiledIR(mm_program);
    printIR(mm_ir_root.get());
  } catch (const std::exception &e) {
    std::cerr << "IR Construction Error: " << e.what() << std::endl;
  }
  std::cout << "--------------------------------------------------------"
            << std::endl
            << std::endl;

  // --- TEST 2: Matrix Addition (2D Loop Nest, Simple Add) ---
  const std::string add_program = R"(
        LOOPS: i=0:N:1, j=0:M:1
        BODY: C[i, j] = A[i, j] + B[i, j]
    )";

  std::cout << "--- TEST 2: Matrix Addition (2D, Simple Add) ---" << std::endl;
  try {
    std::unique_ptr<IRNode> add_ir_root = buildUntiledIR(add_program);
    printIR(add_ir_root.get());
  } catch (const std::exception &e) {
    std::cerr << "IR Construction Error: " << e.what() << std::endl;
  }
  std::cout << "------------------------------------------------" << std::endl
            << std::endl;

  // --- TEST 3: Matrix Transposition (2D, Single Load, Reversed Indices) ---
  const std::string transpose_program = R"(
        LOOPS: i=0:N:1, j=0:M:1
        BODY: C[i, j] = A[j, i]
    )";

  std::cout << "--- TEST 3: Matrix Transposition (2D, Index Swap) ---"
            << std::endl;
  try {
    std::unique_ptr<IRNode> transpose_ir_root =
        buildUntiledIR(transpose_program);
    printIR(transpose_ir_root.get());
  } catch (const std::exception &e) {
    std::cerr << "IR Construction Error: " << e.what() << std::endl;
  }
  std::cout << "-----------------------------------------------------"
            << std::endl
            << std::endl;

  return 0;
}