#pragma once

#include "IR.hpp"
#include <memory>

std::unique_ptr<IRNode> deepCopy(const IRNode *nd);

std::unique_ptr<IRNode> tilingPass(IRNode *nd);