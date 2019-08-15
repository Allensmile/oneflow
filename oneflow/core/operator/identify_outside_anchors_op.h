#ifndef ONEFLOW_CORE_OPERATOR_IDENTIFY_OUTSIDE_ANCHORS_OP_H
#define ONEFLOW_CORE_OPERATOR_IDENTIFY_OUTSIDE_ANCHORS_OP_H

#include "oneflow/core/operator/operator.h"

namespace oneflow {

class IdentifyOutsideAnchorsOp final : public Operator {
 public:
  OF_DISALLOW_COPY_AND_MOVE(IdentifyOutsideAnchorsOp);
  IdentifyOutsideAnchorsOp() = default;
  ~IdentifyOutsideAnchorsOp() override = default;

  void InitFromOpConf() override;
  const PbMessage& GetCustomizedConf() const override;
  void InferBlobDescs(std::function<BlobDesc*(const std::string&)> GetBlobDesc4BnInOp,
                      const ParallelContext* parallel_ctx) const override;

 private:
  virtual void VirtualGenKernelConf(
      std::function<const BlobDesc*(const std::string&)> GetBlobDesc4BnInOp, const ParallelContext*,
      KernelConf*, const OpContext*) const override;
  bool IsInputBlobAllowedModelSplit(const std::string& ibn) const override { return false; }
};

}  // namespace oneflow

#endif  // ONEFLOW_CORE_OPERATOR_IDENTIFY_OUTSIDE_ANCHORS_OP_H
