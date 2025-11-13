#pragma once

#include <cstddef>
#include <vector>

using ConstValue = std::variant<int,       // Int32
                                long long, // Int64
                                float,     // Float32
                                double     // Float64
                                >;

enum class DType {
  Float32,
  Float64,
  Int32,
  Int64,
};

enum class IRNodeType {
  // Structural Nodes
  Loop,
  // Access Nodes
  Load,
  Store,
  // Computation Nodes
  Add,
  Mul,
  Assign,
  // Control Flow / Expression Nodes
  Const,    // For constants like 0, N, T, etc.
  Variable, // For loop indices i, j, ii, jj
  Min,      // Ex MIN(ii + T, N) in tiling bounds
};

class IRNode {
public:
  virtual ~IRNode() = default;
  virtual IRNodeType getType() const = 0;
};

class Tensor {
public:
  Tensor(std::string n, DType d, size_t dims,
         const std::vector<size_t> &extents)
      : name(std::move(n)), dtype_(d), dims_(dims), extents_(extents) {
    if (dims != extents.size()) {
      throw std::invalid_argument("dims must match the size of extents");
    }

    strides_.resize(dims_);

    size_t current_stride = 1;

    for (int i = dims_ - 1; i >= 0; --i) {
      strides_[i] = current_stride;
      // The stride for the next (previous index) dimension
      // is the current stride multiplied by the extent of the current
      // dimension.
      current_stride *= extents_[i];
    }
  }

  std::string name;
  DType dtype_;
  size_t dims_;
  std::vector<size_t> extents_;
  std::vector<size_t> strides_;
};

class Const : public IRNode {
public:
  Const(ConstValue val, DType type) : value_(std::move(val)), dtype_(type) {}

  IRNodeType getType() const override { return IRNodeType::Const; }

  const ConstValue &getValue() const { return value_; }
  DType getDType() const { return dtype_; }

  ConstValue value_;

  DType dtype_;
};

class Variable : public IRNode {
public:
  Variable(std::string name) : name_(std::move(name)) {}

  IRNodeType getType() const override { return IRNodeType::Variable; }

  const std::string &getName() const { return name_; }

  std::string name_;
};

class Min : public IRNode {
public:
  Min(std::unique_ptr<IRNode> one, std::unique_ptr<IRNode> two)
      : operand_one_(std::move(one)), operand_two_(std::move(two)) {}

  IRNodeType getType() const override { return IRNodeType::Min; }

  std::unique_ptr<IRNode> operand_one_;
  std::unique_ptr<IRNode> operand_two_;
};

class Add : public IRNode {
public:
  Add(std::unique_ptr<IRNode> one, std::unique_ptr<IRNode> two)
      : operand_one_(std::move(one)), operand_two_(std::move(two)) {}

  IRNodeType getType() const override { return IRNodeType::Add; }

  std::unique_ptr<IRNode> operand_one_;
  std::unique_ptr<IRNode> operand_two_;
};

class Mul : public IRNode {
public:
  Mul(std::unique_ptr<IRNode> one, std::unique_ptr<IRNode> two)
      : operand_one_(std::move(one)), operand_two_(std::move(two)) {}

  IRNodeType getType() const override { return IRNodeType::Mul; }

  std::unique_ptr<IRNode> operand_one_;
  std::unique_ptr<IRNode> operand_two_;
};

class Load : public IRNode {
public:
  Load(Tensor &t, std::vector<std::unique_ptr<IRNode>> indices)
      : tensor_(t), indices_(std::move(indices)) {}

  IRNodeType getType() const override { return IRNodeType::Load; }

  Tensor &tensor_;
  std::vector<std::unique_ptr<IRNode>> indices_;
};

class Store : public IRNode {
public:
  Store(Tensor &t, std::vector<std::unique_ptr<IRNode>> indices)
      : tensor_(t), indices_(std::move(indices)) {}

  IRNodeType getType() const override { return IRNodeType::Store; }

  Tensor &tensor_;
  std::vector<std::unique_ptr<IRNode>> indices_;
};

class Loop : public IRNode {
public:
  Loop(std::string i, std::unique_ptr<IRNode> lb, std::unique_ptr<IRNode> ub,
       std::unique_ptr<IRNode> step)
      : index_(std::move(i)), lower_bound_(std::move(lb)),
        upper_bound_(std::move(ub)), step_(std::move(step)) {}

  IRNodeType getType() const override { return IRNodeType::Loop; }

  std::string index_;
  std::unique_ptr<IRNode> lower_bound_;
  std::unique_ptr<IRNode> upper_bound_;
  std::unique_ptr<IRNode> step_;
  std::vector<std::unique_ptr<IRNode>> body_;
};

class Assign : public IRNode {
public:
  Assign(std::unique_ptr<IRNode> target, std::unique_ptr<IRNode> value)
      : target_(std::move(target)), value_(std::move(value)) {}

  IRNodeType getType() const override { return IRNodeType::Assign; }

  std::unique_ptr<IRNode> target_;
  std::unique_ptr<IRNode> value_;
};
