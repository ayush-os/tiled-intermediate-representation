#include "TilingPass.hpp"
#include "IR.hpp"
#include <iostream>
#include <memory>

/**
 * @brief Helper function to deep copy a vector of IRNode unique pointers.
 * @param original_nodes The vector to copy from.
 * @return A new vector containing deep copies of the original nodes.
 */
std::vector<std::unique_ptr<IRNode>>
deepCopyVector(const std::vector<std::unique_ptr<IRNode>> &original_nodes) {
  std::vector<std::unique_ptr<IRNode>> new_nodes;
  new_nodes.reserve(original_nodes.size());
  for (const auto &node : original_nodes) {
    new_nodes.push_back(deepCopy(node.get()));
  }
  return new_nodes;
}

/**
 * @brief Performs a deep copy of the given IRNode and its entire subtree.
 *
 * @param nd A pointer to the IRNode to copy. Can be nullptr.
 * @return A unique pointer to the newly created, identical IRNode subtree.
 */
std::unique_ptr<IRNode> deepCopy(const IRNode *nd) {
  if (!nd) {
    return nullptr;
  }

  // Use the virtual method getType() to determine the concrete class
  // and then static_cast to access its members for copying.
  switch (nd->getType()) {
  case IRNodeType::Const: {
    const Const *c = static_cast<const Const *>(nd);
    return std::make_unique<Const>(c->value_, c->dtype_);
  }
  case IRNodeType::Variable: {
    const Variable *v = static_cast<const Variable *>(nd);
    return std::make_unique<Variable>(v->name_);
  }
  case IRNodeType::Min: {
    const Min *m = static_cast<const Min *>(nd);
    return std::make_unique<Min>(deepCopy(m->operand_one_.get()),
                                 deepCopy(m->operand_two_.get()));
  }
  case IRNodeType::Add: {
    const Add *a = static_cast<const Add *>(nd);
    return std::make_unique<Add>(deepCopy(a->operand_one_.get()),
                                 deepCopy(a->operand_two_.get()));
  }
  case IRNodeType::Mul: {
    const Mul *m = static_cast<const Mul *>(nd);
    return std::make_unique<Mul>(deepCopy(m->operand_one_.get()),
                                 deepCopy(m->operand_two_.get()));
  }
  case IRNodeType::Load: {
    const Load *l = static_cast<const Load *>(nd);
    // The Tensor reference is shallow-copied
    // The indices vector must be deep-copied
    return std::make_unique<Load>(l->tensor_, deepCopyVector(l->indices_));
  }
  case IRNodeType::Store: {
    const Store *s = static_cast<const Store *>(nd);
    // The Tensor reference is shallow-copied
    // The indices vector must be deep-copied
    return std::make_unique<Store>(s->tensor_, deepCopyVector(s->indices_));
  }
  case IRNodeType::Assign: {
    const Assign *a = static_cast<const Assign *>(nd);
    return std::make_unique<Assign>(deepCopy(a->target_.get()),
                                    deepCopy(a->value_.get()));
  }
  case IRNodeType::Loop: {
    const Loop *l = static_cast<const Loop *>(nd);

    // Deep copy all children unique pointers
    auto new_lb = deepCopy(l->lower_bound_.get());
    auto new_ub = deepCopy(l->upper_bound_.get());
    auto new_step = deepCopy(l->step_.get());

    // Create the new Loop node
    auto new_loop = std::make_unique<Loop>(
        l->index_, std::move(new_lb), std::move(new_ub), std::move(new_step));

    // Deep copy the body vector and assign it to the new Loop node
    new_loop->body_ = deepCopyVector(l->body_);

    return new_loop;
  }
  default:
    // This should ideally never be reached if all IRNodeType are handled
    std::cerr << "Error: Unknown IRNodeType encountered during deep copy.\n";
    return nullptr;
  }
}

/**
 * @brief Performs 2D loop tiling (i, j) -> (ii, jj, i, j) on a nested loop
 * * @param nd Pointer to the root IRNode (expected to be the outer loop)
 * @return A unique pointer to the newly created, tiled IR subtree
 * WARNING: PROGRAM WILL CRASH IF IR IS NOT STRUCTURED AS 2 NESTED LOOPS
 */
std::unique_ptr<IRNode> tilingPass(IRNode *nd) {
  Loop *og_loop_i = static_cast<Loop *>(nd);
  Loop *og_loop_j = static_cast<Loop *>(og_loop_i->body_.front().get());

  std::unique_ptr<IRNode> cpy = deepCopy(nd);

  Loop *loop_i = static_cast<Loop *>(cpy.get());
  Loop *loop_j = static_cast<Loop *>(loop_i->body_.front().get());

  std::unique_ptr<IRNode> var_ii = std::make_unique<Variable>("ii");
  std::unique_ptr<IRNode> var_jj = std::make_unique<Variable>("jj");
  std::unique_ptr<IRNode> var_t = std::make_unique<Variable>("T");

  std::unique_ptr<IRNode> add_ii_t = std::make_unique<Add>(
      std::move(deepCopy(var_ii.get())), std::move(deepCopy(var_t.get())));
  std::unique_ptr<IRNode> add_jj_t = std::make_unique<Add>(
      std::move(deepCopy(var_jj.get())), std::move(deepCopy(var_t.get())));

  std::unique_ptr<IRNode> loop_i_upper = std::make_unique<Min>(
      std::move(add_ii_t), std::move(deepCopy(og_loop_i->upper_bound_.get())));
  std::unique_ptr<IRNode> loop_j_upper = std::make_unique<Min>(
      std::move(add_jj_t), std::move(deepCopy(og_loop_j->upper_bound_.get())));

  loop_i->lower_bound_ = std::move(deepCopy(var_ii.get()));
  loop_j->lower_bound_ = std::move(deepCopy(var_jj.get()));
  loop_i->upper_bound_ = std::move(loop_i_upper);
  loop_j->upper_bound_ = std::move(loop_j_upper);

  std::unique_ptr<IRNode> loop_jj = std::make_unique<Loop>(
      "jj", std::move(deepCopy(og_loop_j->lower_bound_.get())),
      std::move(deepCopy(og_loop_j->upper_bound_.get())),
      std::move(deepCopy(var_t.get())));
  std::unique_ptr<IRNode> loop_ii = std::make_unique<Loop>(
      "ii", std::move(deepCopy(og_loop_i->lower_bound_.get())),
      std::move(deepCopy(og_loop_i->upper_bound_.get())),
      std::move(deepCopy(var_t.get())));

  static_cast<Loop *>(loop_jj.get())->body_.push_back(std::move(cpy));
  static_cast<Loop *>(loop_ii.get())->body_.push_back(std::move(loop_jj));

  return loop_ii;
}