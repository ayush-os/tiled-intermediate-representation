#include "CodeGenerator.hpp" // Now including the code generation functions
#include "IR.hpp"
#include "IRBuilder.hpp"
#include "TilingPass.hpp"
#include <iostream>

int main() {

  // --- TEST 2: Matrix Addition (2D Loop Nest, Simple Add) ---
  // The tiling pass expects two nested loops.
  const std::string add_program = R"(
        LOOPS: i=0:N:1, j=0:M:1
        BODY: C[i, j] = C[i, j] + A[i, j]
    )";

  std::cout << "--- TEST 2: Matrix Addition (2D, Simple Add) ---" << std::endl;
  try {
    std::unique_ptr<IRNode> add_ir_root = buildUntiledIR(add_program);
    std::unique_ptr<IRNode> tiled_add_ir_root = tilingPass(add_ir_root.get());

    std::cout << "----------------------UNTILED-----------------------"
              << std::endl;
    printIR(add_ir_root.get(), 0);
    std::cout << "----------------------END UNTILED-----------------------"
              << std::endl;

    std::cout << "----------------------TILED-----------------------"
              << std::endl;
    printIR(tiled_add_ir_root.get(), 0);
    std::cout << "----------------------END TILED-----------------------"
              << std::endl;

    // Call the code generator with the Addition IRs (Untiled and Tiled)
    std::cout << "\n>>> Calling generateCodeFiles for Matrix Addition "
                 "Kernels... <<<\n";
    // NOTE: We need a new function or modified logic in CodeGenerator
    // to handle the naming for 'add' vs 'transpose'.
    // For now, we'll call a single function and rely on the IR structure.
    generateCodeFiles(add_ir_root.get(), tiled_add_ir_root.get(), "add");

  } catch (const std::exception &e) {
    std::cerr << "IR Construction Error (Add): " << e.what() << std::endl;
  }
  std::cout << "------------------------------------------------" << std::endl
            << std::endl;

  // ----------------------------------------------------------------------------

  // --- TEST 3: Matrix Transposition (2D, Single Load, Reversed Indices) ---
  const std::string transpose_program = R"(
        LOOPS: i=0:N:1, j=0:M:1
        BODY: C[i, j] = A[j, i]
    )";

  std::unique_ptr<IRNode> transpose_ir_root = nullptr;
  std::unique_ptr<IRNode> tiled_transpose_ir_root = nullptr;

  std::cout << "--- TEST 3: Matrix Transposition (2D, Index Swap) ---"
            << std::endl;
  try {
    transpose_ir_root = buildUntiledIR(transpose_program);
    tiled_transpose_ir_root = tilingPass(transpose_ir_root.get());

    std::cout << "----------------------UNTILED-----------------------"
              << std::endl;
    printIR(transpose_ir_root.get(), 0);
    std::cout << "----------------------END UNTILED-----------------------"
              << std::endl;

    std::cout << "----------------------TILED-----------------------"
              << std::endl;
    printIR(tiled_transpose_ir_root.get(), 0);
    std::cout << "----------------------END TILED-----------------------"
              << std::endl;

    // Call the code generator with the Transpose IRs (Untiled and Tiled)
    std::cout
        << "\n>>> Calling generateCodeFiles for Transpose Kernels... <<<\n";
    generateCodeFiles(transpose_ir_root.get(), tiled_transpose_ir_root.get(),
                      "transpose");

  } catch (const std::exception &e) {
    std::cerr << "IR Construction Error (Transpose): " << e.what() << std::endl;
  }
  std::cout << "-----------------------------------------------------"
            << std::endl
            << std::endl;

  return 0;
}