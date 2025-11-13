#include "IR.hpp"
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>

// --- I. Global Setup (Simulating Compiler Environment) ---

// Defining Tensors with names
Tensor TensorA("A", DType::Float32, 2, {1024, 1024});
Tensor TensorB("B", DType::Float32, 2, {1024, 1024});
Tensor TensorC("C", DType::Float32, 2, {1024, 1024});

// Map to look up Tensors by name during parsing
std::map<std::string, Tensor *> TensorMap = {
    {"A", &TensorA}, {"B", &TensorB}, {"C", &TensorC}};

// --- II. Helper Functions ---

// Simple string splitting utility
std::vector<std::string> split(const std::string &s, char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(s);
  while (std::getline(tokenStream, token, delimiter)) {
    if (!token.empty()) {
      tokens.push_back(token);
    }
  }
  return tokens;
}

// Function to clean up spaces and parentheses from a string
std::string clean_expr(std::string s) {
  s.erase(std::remove_if(s.begin(), s.end(), ::isspace), s.end());
  // Only remove surrounding parentheses if they enclose the *entire* expression
  if (s.front() == '(' && s.back() == ')') {
    // Simple check: This doesn't handle nested parentheses fully but works for
    // our fixed syntax
    return s.substr(1, s.length() - 2);
  }
  return s;
}

// Helper to parse index list and tensor access (shared logic for Load/Store)
std::tuple<Tensor *, std::vector<std::unique_ptr<IRNode>>>
parseTensorAccess(const std::string &access_str) {
  // Expected format: TENSOR_NAME[IDX1, IDX2, ...]
  size_t open_bracket = access_str.find('[');
  size_t close_bracket = access_str.find(']');

  if (open_bracket == std::string::npos || close_bracket == std::string::npos) {
    throw std::runtime_error("Invalid array access format in body.");
  }

  std::string tensor_name = access_str.substr(0, open_bracket);
  std::string indices_str =
      access_str.substr(open_bracket + 1, close_bracket - open_bracket - 1);

  Tensor *t = TensorMap.at(tensor_name);

  std::vector<std::string> index_vars = split(indices_str, ',');
  std::vector<std::unique_ptr<IRNode>> indices;

  for (const auto &var : index_vars) {
    // Indices are assumed to be simple Variables (like "i", "j") for this
    // builder
    indices.push_back(std::make_unique<Variable>(var));
  }

  return {t, std::move(indices)};
}

// 1. Builds a Load node: e.g., A[i, j] -> Load(TensorA, Variable("i"),
// Variable("j"))
std::unique_ptr<IRNode> parseLoad(const std::string &access_str) {
  auto [t, indices] = parseTensorAccess(access_str);
  return std::make_unique<Load>(*t, std::move(indices));
}

// 2. Builds a Store node: e.g., C[i, j] -> Store(TensorC, Variable("i"),
// Variable("j"))
std::unique_ptr<IRNode> parseStore(const std::string &access_str) {
  auto [t, indices] = parseTensorAccess(access_str);
  return std::make_unique<Store>(*t, std::move(indices));
}

// 3. Builds the Right-Hand Side expression tree recursively (e.g., A[i, j] +
// B[i, j])
std::unique_ptr<IRNode> parseExpression(const std::string &expr_str) {
  std::string cleaned = clean_expr(expr_str);

  // --- Base Case: Single Operand (Load) ---
  if (cleaned.find('+') == std::string::npos &&
      cleaned.find('*') == std::string::npos) {
    return parseLoad(cleaned);
  }

  // --- Recursive Case: Find Main Operator (Precedence: + then *) ---

  // Look for '+' (lowest precedence)
  size_t op_pos = cleaned.rfind('+');
  if (op_pos != std::string::npos) {
    std::string left = cleaned.substr(0, op_pos);
    std::string right = cleaned.substr(op_pos + 1);
    return std::make_unique<Add>(parseExpression(left), parseExpression(right));
  }

  // Look for '*' (next precedence)
  op_pos = cleaned.rfind('*');
  if (op_pos != std::string::npos) {
    std::string left = cleaned.substr(0, op_pos);
    std::string right = cleaned.substr(op_pos + 1);
    return std::make_unique<Mul>(parseExpression(left), parseExpression(right));
  }

  // Should only be reached if expression was surrounded by parentheses we
  // removed
  return parseExpression(cleaned);
}

// 4. Main Builder: Combines Loop structure and the Assignment statement
std::unique_ptr<IRNode> buildUntiledIR(const std::string &input_program) {

  // --- Step 1: Parse Loops and Body Strings ---
  size_t loops_start = input_program.find("LOOPS:");
  size_t body_start = input_program.find("BODY:");

  std::string loops_str =
      input_program.substr(loops_start + 6, body_start - (loops_start + 6));
  std::string body_str = input_program.substr(body_start + 5);

  // --- Step 2: Parse Assignment Statement (BODY) ---
  size_t assign_pos = body_str.find('=');
  std::string target_str = clean_expr(body_str.substr(0, assign_pos));
  std::string value_str = clean_expr(body_str.substr(assign_pos + 1));

  // Use parseStore for the LHS target
  auto store_target_node = parseStore(target_str);

  // Value (RHS): The complex expression tree
  auto value_expr_root = parseExpression(value_str);

  // Create the core Assign statement
  auto assign_stmt = std::make_unique<Assign>(std::move(store_target_node),
                                              std::move(value_expr_root));

  // --- Step 3: Parse and Nest Loops ---
  std::vector<std::string> loop_tokens = split(loops_str, ',');

  std::unique_ptr<IRNode> current_body = std::move(assign_stmt);

  for (int i = loop_tokens.size() - 1; i >= 0; --i) {
    std::vector<std::string> parts = split(loop_tokens[i], '=');
    std::string var = parts[0];

    std::vector<std::string> bounds = split(parts[1], ':');

    // Lambda to parse bounds: number -> Const, anything else -> Variable
    auto parse_bound = [](const std::string &s) -> std::unique_ptr<IRNode> {
      if (std::all_of(s.begin(), s.end(), ::isdigit)) {
        return std::make_unique<Const>(std::stoi(s), DType::Int32);
      }
      return std::make_unique<Variable>(s);
    };

    auto new_loop =
        std::make_unique<Loop>(var,                    // index variable
                               parse_bound(bounds[0]), // lower bound (LB)
                               parse_bound(bounds[1]), // upper bound (UB)
                               parse_bound(bounds[2])  // step (STEP)
        );

    new_loop->body_.push_back(std::move(current_body));
    current_body = std::move(new_loop);
  }

  return current_body;
}

// ====================================================================
// TRAVERSAL AND MAIN FUNCTION (Phase 2 Verification)
// ====================================================================

// --- Utility for Indentation ---
std::string indent_level(int depth) { return std::string(depth * 4, ' '); }

// --- Recursive Traversal Function ---
void printIR(const IRNode *node, int depth = 0) {
  if (!node)
    return;

  std::cout << indent_level(depth);

  switch (node->getType()) {
  case IRNodeType::Loop: {
    const Loop *loop = static_cast<const Loop *>(node);
    std::cout << "LOOP: for " << loop->index_ << " = ";
    std::cout << "[LB] to [UB] step [STEP]" << std::endl;

    for (const auto &child : loop->body_) {
      printIR(child.get(), depth + 1);
    }
    break;
  }

  case IRNodeType::Assign: {
    std::cout << "ASSIGN" << std::endl;
    printIR(static_cast<const Assign *>(node)->target_.get(), depth + 1);
    printIR(static_cast<const Assign *>(node)->value_.get(), depth + 1);
    break;
  }

  case IRNodeType::Load: {
    const Load *load = static_cast<const Load *>(node);
    std::cout << "LOAD: " << load->tensor_.name << "[";
    for (size_t i = 0; i < load->indices_.size(); ++i) {
      const Variable *var =
          dynamic_cast<const Variable *>(load->indices_[i].get());
      std::cout << (var ? var->getName() : "?");
      if (i < load->indices_.size() - 1)
        std::cout << ", ";
    }
    std::cout << "]" << std::endl;
    break;
  }

  case IRNodeType::Store: {
    const Store *store = static_cast<const Store *>(node);
    std::cout << "STORE (Target): " << store->tensor_.name << "[";
    for (size_t i = 0; i < store->indices_.size(); ++i) {
      const Variable *var =
          dynamic_cast<const Variable *>(store->indices_[i].get());
      std::cout << (var ? var->getName() : "?");
      if (i < store->indices_.size() - 1)
        std::cout << ", ";
    }
    std::cout << "]" << std::endl;
    break;
  }

  case IRNodeType::Add:
  case IRNodeType::Mul:
  case IRNodeType::Min: {
    // Note: Add, Mul, Min share the two-operand structure
    const Add *binary = static_cast<const Add *>(node);
    std::string op = (node->getType() == IRNodeType::Add)   ? "ADD"
                     : (node->getType() == IRNodeType::Mul) ? "MUL"
                                                            : "MIN";
    std::cout << op << std::endl;
    printIR(binary->operand_one_.get(), depth + 1);
    printIR(binary->operand_two_.get(), depth + 1);
    break;
  }

  case IRNodeType::Const: {
    const Const *constant = static_cast<const Const *>(node);
    std::string val_str = std::visit(
        [](auto &&arg) -> std::string {
          using T = std::decay_t<decltype(arg)>;
          if constexpr (std::is_integral_v<T>) {
            return std::to_string(arg);
          } else if constexpr (std::is_floating_point_v<T>) {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << arg;
            return ss.str();
          } else {
            return "Unknown";
          }
        },
        constant->getValue());
    std::cout << "CONST: " << val_str << std::endl;
    break;
  }

  case IRNodeType::Variable: {
    const Variable *var = static_cast<const Variable *>(node);
    std::cout << "VAR: " << var->getName() << std::endl;
    break;
  }

  default:
    std::cout << "UNKNOWN_NODE" << std::endl;
  }
}

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