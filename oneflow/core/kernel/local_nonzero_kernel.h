#ifndef ONEFLOW_CORE_OPERATOR_LOCAL_NONZERO_KERNEL_OP_H_
#define ONEFLOW_CORE_OPERATOR_LOCAL_NONZERO_KERNEL_OP_H_

#include "oneflow/core/kernel/kernel.h"

namespace oneflow {

template<DeviceType device_type, typename T>
class LocalNonzeroKernel final : public KernelIf<device_type> {
 public:
  OF_DISALLOW_COPY_AND_MOVE(LocalNonzeroKernel);
  LocalNonzeroKernel() = default;
  ~LocalNonzeroKernel() = default;

 private:
  void ForwardDataContent(const KernelCtx&,
                          std::function<Blob*(const std::string&)>) const override;
  void ForwardDim0ValidNum(const KernelCtx&,
                           std::function<Blob*(const std::string&)>) const override;
};

template<typename T>
void CpuNonzero(DeviceCtx* ctx, const Blob* in_blob, Blob* out_blob);

template<typename T>
void GpuNonzero(DeviceCtx* ctx, const Blob* in_blob, Blob* num_nonzero_blob, Blob* shape_blob,
                Blob* out_blob);

}  // namespace oneflow

#endif  // ONEFLOW_CORE_OPERATOR_LOCAL_NONZERO_KERNEL_OP_H_
