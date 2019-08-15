#include "oneflow/core/kernel/logical_and_kernel.h"

namespace oneflow {

template<DeviceType device_type, typename T>
void LogicalAndKernel<device_type, T>::ForwardDataContent(
    const KernelCtx& ctx, std::function<Blob*(const std::string&)> BnInOp2Blob) const {
  const Blob* lhs_blob = BnInOp2Blob("lhs");
  const Blob* rhs_blob = BnInOp2Blob("rhs");
  Blob* out_blob = BnInOp2Blob("out");

  const Shape shape = lhs_blob->shape();
  CHECK_EQ(shape, rhs_blob->shape());
  CHECK_EQ(shape, out_blob->shape());
  LogicalAndUtil<device_type, T>::Forward(ctx.device_ctx, shape.elem_cnt(), lhs_blob->dptr<T>(),
                                          rhs_blob->dptr<T>(), out_blob->mut_dptr<T>());
}

template<DeviceType device_type, typename T>
void LogicalAndKernel<device_type, T>::ForwardDim0ValidNum(
    const KernelCtx&, std::function<Blob*(const std::string&)> BnInOp2Blob) const {
  const Blob* lhs_blob = BnInOp2Blob("lhs");
  const Blob* rhs_blob = BnInOp2Blob("rhs");
  CHECK_EQ(lhs_blob->shape(), rhs_blob->shape());
  BnInOp2Blob("out")->set_dim0_valid_num(0, lhs_blob->shape().At(0));
}

template<DeviceType device_type, typename T>
void LogicalAndKernel<device_type, T>::ForwardInstanceShape(
    const KernelCtx& ctx, std::function<Blob*(const std::string&)> BnInOp2Blob) const {
  const Blob* lhs_blob = BnInOp2Blob("lhs");
  const Blob* rhs_blob = BnInOp2Blob("rhs");
  CHECK_EQ(lhs_blob->shape(), rhs_blob->shape());
  BnInOp2Blob("out")->set_instance_shape(lhs_blob->instance_shape());
}

template<typename T>
struct LogicalAndUtil<DeviceType::kCPU, T> {
  static void Forward(DeviceCtx* ctx, const int64_t elem_cnt, const T* lhs_ptr, const T* rhs_ptr,
                      T* out_ptr) {
    UNIMPLEMENTED();
  }
};

ADD_DEFAULT_KERNEL_CREATOR(OperatorConf::kLogicalAndConf, LogicalAndKernel, INT_DATA_TYPE_SEQ);

}  // namespace oneflow
