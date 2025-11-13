#pragma once

#include <cstddef>
#include <vector>

using ConstValue = std::variant<
    int,       // Int32
    long long, // Int64
    float,     // Float32
    double     // Float64
    >;

enum class DType
{
    Float32,
    Float64,
    Int32,
    Int64,
};

enum class IRNodeType
{
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
    Const,     // For constants like 0, N, T, etc.
    Variable,  // For loop indices i, j, ii, jj
    Min,       // Ex MIN(ii + T, N) in tiling bounds
    ScalerRef, // L-type for Assign Nodes
};

class Tensor
{
public:
private:
    DType dtype_;
    size_t dims_;
    std::vector<size_t> extents_;
    std::vector<size_t> strides_;
};

class IRNode
{
public:
    virtual ~IRNode() = default;

    virtual IRNodeType getType() const = 0;
};

class Load : public IRNode
{
public:
    Load(Tensor &t, std::vector<std::unique_ptr<IRNode>> indices)
        : tensor_(t), indices_(std::move(indices)) {}

    IRNodeType getType() const override { return IRNodeType::Load; }

private:
    Tensor &tensor_;
    std::vector<std::unique_ptr<IRNode>> indices_;
};

class Store : public IRNode
{
public:
    Store(Tensor &t, std::vector<std::unique_ptr<IRNode>> indices)
        : tensor_(t), indices_(std::move(indices)) {}

    IRNodeType getType() const override { return IRNodeType::Store; }

private:
    Tensor &tensor_;
    std::vector<std::unique_ptr<IRNode>> indices_;
};

class Loop : public IRNode
{
public:
    Loop(std::string i, std::unique_ptr<IRNode> lb, std::unique_ptr<IRNode> ub, std::unique_ptr<IRNode> step)
        : index_(std::move(i)), lower_bound_(std::move(lb)), upper_bound_(std::move(ub)), step_(std::move(step)) {}

    IRNodeType getType() const override { return IRNodeType::Loop; }

    std::vector<std::unique_ptr<IRNode>> body_;

private:
    std::string index_;
    std::unique_ptr<IRNode> lower_bound_;
    std::unique_ptr<IRNode> upper_bound_;
    std::unique_ptr<IRNode> step_;
};

class Add : public IRNode
{
public:
    Add(std::unique_ptr<IRNode> one, std::unique_ptr<IRNode> two) : operand_one_(std::move(one)), operand_two_(std::move(two)) {}

    IRNodeType getType() const override { return IRNodeType::Add; }

private:
    std::unique_ptr<IRNode> operand_one_;
    std::unique_ptr<IRNode> operand_two_;
};

class Mul : public IRNode
{
public:
    Mul(std::unique_ptr<IRNode> one, std::unique_ptr<IRNode> two) : operand_one_(std::move(one)), operand_two_(std::move(two)) {}

    IRNodeType getType() const override { return IRNodeType::Mul; }

private:
    std::unique_ptr<IRNode> operand_one_;
    std::unique_ptr<IRNode> operand_two_;
};

class Assign : public IRNode
{
public:
    Assign(std::unique_ptr<IRNode> target, std::unique_ptr<IRNode> value)
        : target_(std::move(target)), value_(std::move(value)) {}

    IRNodeType getType() const override { return IRNodeType::Assign; }

private:
    std::unique_ptr<IRNode> target_;
    std::unique_ptr<IRNode> value_;
};

class Const : public IRNode
{
public:
    Const(ConstValue val, DType type) : value_(std::move(val)), dtype_(type) {}

    IRNodeType getType() const override { return IRNodeType::Const; }

    const ConstValue &getValue() const { return value_; }
    DType getDType() const { return dtype_; }

private:
    ConstValue value_;

    DType dtype_;
};

class Variable : public IRNode
{
public:
    Variable(std::string name) : name_(std::move(name)) {}

    IRNodeType getType() const override { return IRNodeType::Variable; }

    const std::string &getName() const { return name_; }

private:
    std::string name_;
};

class Min : public IRNode
{
public:
    Min(std::unique_ptr<IRNode> one, std::unique_ptr<IRNode> two) : operand_one_(std::move(one)), operand_two_(std::move(two)) {}

    IRNodeType getType() const override { return IRNodeType::Min; }

private:
    std::unique_ptr<IRNode> operand_one_;
    std::unique_ptr<IRNode> operand_two_;
};

class ScalarRef : public IRNode
{
public:
    ScalarRef(std::string name) : name_(std::move(name)) {}

    IRNodeType getType() const override { return IRNodeType::ScalerRef; }

    const std::string &getName() const { return name_; }

private:
    std::string name_;
};