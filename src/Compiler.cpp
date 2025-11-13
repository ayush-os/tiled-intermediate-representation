#include "IRBuilder.hpp"
#include "TilingPass.hpp"
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

  Tensor A("A", DType::Float32, 2, {10, 10});

  // Create a simple IR: A[i, j] = A[i, j] + 1
  // Loop i { Loop j { A[i, j] = A[i, j] + 1 } }

  // Constants used for bounds and addition
  auto one_const = std::make_unique<Const>(ConstValue(1), DType::Int32);
  auto zero_const = std::make_unique<Const>(ConstValue(0), DType::Int32);
  auto ten_const = std::make_unique<Const>(ConstValue(10), DType::Int32);

  // Variable for index expressions
  auto i_var = std::make_unique<Variable>("i");
  auto j_var = std::make_unique<Variable>("j");

  // Load: A[i, j] (need copies of i_var and j_var for indices)
  std::vector<std::unique_ptr<IRNode>> load_indices;
  load_indices.push_back(deepCopy(i_var.get()));
  load_indices.push_back(deepCopy(j_var.get()));
  auto load_a = std::make_unique<Load>(A, std::move(load_indices));

  // Value: A[i, j] + 1
  auto add_op =
      std::make_unique<Add>(std::move(load_a), deepCopy(one_const.get()));

  // Target (for Assign): Store A[i, j]
  std::vector<std::unique_ptr<IRNode>> store_indices;
  store_indices.push_back(deepCopy(i_var.get()));
  store_indices.push_back(deepCopy(j_var.get()));
  auto store_a = std::make_unique<Store>(A, std::move(store_indices));

  // Inner statement: Assign (Store A[i, j], Add)
  auto assign_stmt =
      std::make_unique<Assign>(std::move(store_a), std::move(add_op));

  // Loop j { Assign }
  auto j_loop = std::make_unique<Loop>("j", deepCopy(zero_const.get()),
                                       deepCopy(ten_const.get()),
                                       deepCopy(one_const.get()));
  j_loop->body_.push_back(std::move(assign_stmt));

  // Loop i { Loop j }
  auto i_loop = std::make_unique<Loop>(
      "i", std::make_unique<Const>(ConstValue(0), DType::Int32),
      std::make_unique<Const>(ConstValue(10), DType::Int32),
      std::make_unique<Const>(ConstValue(1), DType::Int32));
  i_loop->body_.push_back(std::move(j_loop));

  // The original root IR
  std::unique_ptr<IRNode> root = std::move(i_loop);

  std::cout << "--- Original IR Tree ---\n";
  printIR(root.get(), 0);

  // DEEP COPY CALL
  std::unique_ptr<IRNode> root_copy = deepCopy(root.get());

  std::cout << "\n--- Deep Copy IR Tree ---\n";
  printIR(root_copy.get(), 0);

  // Simple pointer check for deep copy verification (The root pointers must be
  // different)
  std::cout << "\nVerification: \n";
  std::cout << "Original Root Address: " << root.get() << "\n";
  std::cout << "Copy Root Address:     " << root_copy.get() << "\n";

  // Also check a child node pointer to confirm deep copy
  // Since 'root' is a Loop, check its lower_bound pointer
  const Loop *original_loop = static_cast<const Loop *>(root.get());
  const Loop *copied_loop = static_cast<const Loop *>(root_copy.get());

  std::cout << "Original LB Address:   " << original_loop->lower_bound_.get()
            << "\n";
  std::cout << "Copy LB Address:       " << copied_loop->lower_bound_.get()
            << "\n";

  std::cout << "Deep copy successful if ALL addresses are different.\n";

  return 0;
}