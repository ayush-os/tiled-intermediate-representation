#include "CodeGenerator.hpp"
#include "IR.hpp"

// --- Utility Functions (for Code Generation) ---

/**
 * @brief Utility for Indentation
 */
std::string indent_level_code_gen(int depth) {
  return std::string(depth * 4, ' ');
}

/**
 * @brief Recursively generates the C++ code for an expression (Const, Variable,
 * Add, Mul, Min, Load).
 *
 * @param node A pointer to the root of the expression IRNode.
 * @return A string containing the C++ representation of the expression.
 */
std::string generateExpression(const IRNode *node) {
  if (!node)
    return "/* NULL_EXPR */";

  switch (node->getType()) {
  case IRNodeType::Const: {
    const Const *constant = static_cast<const Const *>(node);
    return std::visit(
        [](auto &&arg) -> std::string { return std::to_string(arg); },
        constant->getValue());
  }
  case IRNodeType::Variable: {
    return static_cast<const Variable *>(node)->getName();
  }
  case IRNodeType::Add: {
    const Add *a = static_cast<const Add *>(node);
    return "(" + generateExpression(a->operand_one_.get()) + " + " +
           generateExpression(a->operand_two_.get()) + ")";
  }
  case IRNodeType::Mul: {
    const Mul *m = static_cast<const Mul *>(node);
    return "(" + generateExpression(m->operand_one_.get()) + " * " +
           generateExpression(m->operand_two_.get()) + ")";
  }
  case IRNodeType::Min: {
    const Min *m = static_cast<const Min *>(node);
    // Using C++ standard library min function
    return "std::min(" + generateExpression(m->operand_one_.get()) + ", " +
           generateExpression(m->operand_two_.get()) + ")";
  }
  case IRNodeType::Load: {
    const Load *load = static_cast<const Load *>(node);
    // NOTE: This uses the multi-dimensional syntax A[i, j], which needs to be
    // manually mapped to 1D flat array indexing A[i * N + j] in a production
    // system.
    std::string s = load->tensor_.name + "[";
    for (size_t i = 0; i < load->indices_.size(); ++i) {
      s += generateExpression(load->indices_[i].get());
      if (i < load->indices_.size() - 1)
        s += ", ";
    }
    s += "]";
    return s;
  }
  default:
    // Other node types are statements, not expressions that return a value
    return "/* UNHANDLED_EXPR_TYPE */";
  }
}

/**
 * @brief Recursively generates C++ code from the IR tree into an output stream.
 *
 * @param root A pointer to the root of the IRNode subtree to generate code for.
 * @param depth The current indentation level.
 * @param os The output stream to write the generated C++ code to.
 */
void codeGeneration(const IRNode *root, int depth, std::ostream &os) {
  if (!root)
    return;

  os << indent_level_code_gen(depth);

  switch (root->getType()) {
  case IRNodeType::Loop: {
    const Loop *loop = static_cast<const Loop *>(root);

    std::string lb_expr = generateExpression(loop->lower_bound_.get());
    std::string ub_expr = generateExpression(loop->upper_bound_.get());
    std::string step_expr = generateExpression(loop->step_.get());

    // Assuming loop index is an 'int' and step is positive for i += step format
    os << "for (int " << loop->index_ << " = " << lb_expr << "; "
       << loop->index_ << " < " << ub_expr << "; " << loop->index_
       << " += " << step_expr << ") {\n";

    // Recursively generate the body
    for (const auto &child : loop->body_) {
      codeGeneration(child.get(), depth + 1, os);
    }

    os << indent_level_code_gen(depth) << "}\n";
    break;
  }

  case IRNodeType::Assign: {
    const Assign *assign = static_cast<const Assign *>(root);

    std::string target_expr;
    if (assign->target_->getType() == IRNodeType::Store) {
      // Treat Store as the left-hand side assignment target
      const Store *store = static_cast<const Store *>(assign->target_.get());
      std::string access_str = store->tensor_.name + "[";
      for (size_t i = 0; i < store->indices_.size(); ++i) {
        access_str += generateExpression(store->indices_[i].get());
        if (i < store->indices_.size() - 1)
          access_str += ", ";
      }
      access_str += "]";
      target_expr = access_str;
    } else if (assign->target_->getType() == IRNodeType::Variable) {
      target_expr =
          static_cast<const Variable *>(assign->target_.get())->getName();
    } else {
      target_expr = "/* INVALID_TARGET */";
    }

    // Value is the recursive expression generation
    std::string value_expr = generateExpression(assign->value_.get());

    os << target_expr << " = " << value_expr << ";\n";
    break;
  }

  default:
    // Skip all other nodes as they are handled as parts of expressions (Load,
    // Const, etc.)
    break;
  }
}

void generateCodeFiles(const IRNode *untiled_root, const IRNode *tiled_root,
                       const std::string &kernel_type) {

  // Helper function to print the code to a given stream (now always std::cout)
  auto print_code = [&](const std::string &type_name, const IRNode *root,
                        const std::string &kernel_name_base, std::ostream &os) {
    std::string full_kernel_name = type_name + "_" + kernel_name_base;
    os << "\n======================================================\n";
    os << ">>> GENERATED C++ CODE: " << type_name << " " << kernel_name_base
       << " KERNEL <<<\n";
    os << "======================================================\n\n";

    // --- Standard C++ Boilerplate Header ---
    os << "#include <algorithm>\n";
    os << "#include <iostream>\n";
    os << "#include <cmath>\n\n";

    os << "/**\n";
    os << " * Generated kernel: " << full_kernel_name << "\n";
    os << " */\n";
    // The function signature assumes 1D flat array pointers and a size N
    os << "void " << full_kernel_name << "(\n";
    os << "    float *A, float *B, float *C, // Array data pointers\n";
    os << "    int N) {\n"; // N is the dimension size (e.g., 1024)

    // --- Kernel Body Generation ---
    codeGeneration(root, 1, os);

    os << "}\n\n";

    // Example boilerplate for testing
    os << "/*\n";
    os << "int main() {\n";
    os << "    // Example usage requires initializing tensors and calling the "
          "kernel\n";
    os << "    return 0;\n";
    os << "}\n";
    os << "*/\n";
  };

  // 1. Generate Untiled Code
  print_code("untiled", untiled_root, kernel_type, std::cout);

  // 2. Generate Tiled Code
  print_code("tiled", tiled_root, kernel_type, std::cout);

  std::cout << "\n[CodeGenerator] Successfully generated C++ code and printed "
               "it to the console for **"
            << kernel_type << "** kernels.\n";
}