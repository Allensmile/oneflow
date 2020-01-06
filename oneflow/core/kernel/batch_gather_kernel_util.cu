#include "oneflow/core/kernel/batch_gather_kernel_util.h"
#include "oneflow/core/kernel/kernel_util.cuh"
#include <assert.h>

namespace oneflow {

namespace {

Shape GetFlatShape(const ShapeView& shape, const int64_t axis) {
  CHECK_GT(shape.NumAxes(), 0);
  CHECK_GE(axis, 0);
  CHECK_LT(axis, shape.NumAxes());
  return Shape({shape.Count(0, axis), shape.At(axis), shape.Count(axis + 1)});
}

template<typename K, typename IDX>
__device__ int64_t GetInOffset(const IDX out_offset, const K* indices, const IDX indices_num,
                               const IDX instance_size, const IDX gather_dim_size,
                               const IDX in_batch_size, const IDX out_batch_size) {
  const IDX batch_idx = out_offset / out_batch_size;
  const IDX indices_idx = out_offset % out_batch_size / instance_size;
  const IDX inner_idx = out_offset % instance_size;
  const K idx = indices[batch_idx * indices_num + indices_idx];
  assert(idx >= 0 && idx < gather_dim_size);
  return batch_idx * in_batch_size + idx * instance_size + inner_idx;
}

template<typename T, typename K, typename IDX>
__global__ void BatchGatherForwardGpu(const IDX elem_cnt, const T* in, const K* indices,
                                      const IDX indices_num, const IDX instance_size,
                                      const IDX gather_dim_size, T* out) {
  const IDX in_batch_size = gather_dim_size * instance_size;
  const IDX out_batch_size = indices_num * instance_size;
  CUDA_1D_KERNEL_LOOP_T(IDX, i, elem_cnt) {
    out[i] = in[GetInOffset<K, IDX>(i, indices, indices_num, instance_size, gather_dim_size,
                                    in_batch_size, out_batch_size)];
  }
}

}  // namespace

template<typename T, typename K>
struct BatchGatherKernelUtilImpl<DeviceType::kGPU, T, K> final {
  static void Forward(DeviceCtx* ctx, const T* in, const K* indices, const Shape& flat_out_shape,
                      const int64_t gather_dim_size, T* out);
};

template<typename T, typename K>
void BatchGatherKernelUtilImpl<DeviceType::kGPU, T, K>::Forward(DeviceCtx* ctx, const T* in,
                                                                const K* indices,
                                                                const Shape& flat_out_shape,
                                                                const int64_t gather_dim_size,
                                                                T* out) {
  const int64_t batch_num = flat_out_shape.At(0);
  const int64_t indices_num = flat_out_shape.At(1);
  const int64_t instance_size = flat_out_shape.At(2);
  const int64_t out_elem_cnt = batch_num * indices_num * instance_size;
  const int64_t in_elem_cnt = batch_num * gather_dim_size * instance_size;
  if (std::max(out_elem_cnt, in_elem_cnt) > GetMaxVal<int32_t>() / 2) {
    BatchGatherForwardGpu<T, K, int64_t>
        <<<BlocksNum4ThreadsNum(out_elem_cnt), kCudaThreadsNumPerBlock, 0, ctx->cuda_stream()>>>(
            out_elem_cnt, in, indices, indices_num, instance_size, gather_dim_size, out);
  } else {
    BatchGatherForwardGpu<T, K, int32_t>
        <<<BlocksNum4ThreadsNum(out_elem_cnt), kCudaThreadsNumPerBlock, 0, ctx->cuda_stream()>>>(
            out_elem_cnt, in, indices, indices_num, instance_size, gather_dim_size, out);
  }
}

#define INSTANTIATE_BATCH_GATHER_KERNEL_UTIL_IMPL_GPU(in_type_pair, index_type_pair)          \
  template struct BatchGatherKernelUtilImpl<DeviceType::kGPU, OF_PP_PAIR_FIRST(in_type_pair), \
                                            OF_PP_PAIR_FIRST(index_type_pair)>;
OF_PP_SEQ_PRODUCT_FOR_EACH_TUPLE(INSTANTIATE_BATCH_GATHER_KERNEL_UTIL_IMPL_GPU,
                                 FLOATING_DATA_TYPE_SEQ, INDEX_DATA_TYPE_SEQ);
#undef INSTANTIATE_BATCH_GATHER_KERNEL_UTIL_IMPL_GPU

}  // namespace oneflow
