#include "IR.hpp"
#include "IRBuilder.hpp"
#include "TilingPass.hpp"
#include <iostream>

int main() {
  // --- TEST: Matrix Addition (2D Loop Nest, Simple Add) ---
  const std::string add_program = R"(
        LOOPS: i=0:N:1, j=0:M:1
        BODY: C[i, j] = A[i, j] + B[i, j]
    )";

  std::cout << "--- TEST 2: Matrix Addition (2D, Simple Add) ---" << std::endl;
  try {
    std::unique_ptr<IRNode> add_ir_root = buildUntiledIR(add_program);
    std::unique_ptr<IRNode> tiled_add_ir_root = tilingPass(add_ir_root.get());

    std::cout << "----------------------UNTILED-----------------------"
              << std::endl;
    printIR(add_ir_root.get());
    std::cout << "----------------------END UNTILED-----------------------"
              << std::endl;

    std::cout << "----------------------TILED-----------------------"
              << std::endl;
    printIR(tiled_add_ir_root.get());
    std::cout << "----------------------END TILED-----------------------"
              << std::endl;

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
    std::unique_ptr<IRNode> tiled_transpose_ir_root =
        tilingPass(transpose_ir_root.get());

    std::cout << "----------------------UNTILED-----------------------"
              << std::endl;
    printIR(transpose_ir_root.get());
    std::cout << "----------------------END UNTILED-----------------------"
              << std::endl;

    std::cout << "----------------------TILED-----------------------"
              << std::endl;
    printIR(tiled_transpose_ir_root.get());
    std::cout << "----------------------END TILED-----------------------"
              << std::endl;

  } catch (const std::exception &e) {
    std::cerr << "IR Construction Error: " << e.what() << std::endl;
  }
  std::cout << "-----------------------------------------------------"
            << std::endl
            << std::endl;

  return 0;
}