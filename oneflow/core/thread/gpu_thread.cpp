#include "oneflow/core/thread/gpu_thread.h"
#include "oneflow/core/device/cuda_stream_handle.h"

namespace oneflow {

#ifdef WITH_CUDA

GpuThread::GpuThread(int64_t thrd_id, int64_t dev_id) {
  set_thrd_id(thrd_id);
  mut_actor_thread() = std::thread([this, dev_id, thrd_id]() {
    CudaCheck(cudaSetDevice(dev_id));
    ThreadCtx ctx;
    int32_t lower;
    int32_t upper;
    CudaCheck(cudaDeviceGetStreamPriorityRange(&lower, &upper));
    int32_t priority;
    if (thrd_id == Global<IDMgr>::Get()->GetGpuMixThrdId(dev_id)) {
      priority = upper;
    } else {
      priority = lower;
    }
    ctx.g_cuda_stream.reset(new CudaStreamHandle(&cb_event_chan_, priority));
    ctx.cb_event_chan = &cb_event_chan_;
    PollMsgChannel(ctx);
  });
  cb_event_poller_ = std::thread([this, dev_id]() {
    CudaCheck(cudaSetDevice(dev_id));
    CudaCBEvent cb_event;
    while (cb_event_chan_.Receive(&cb_event) == kChannelStatusSuccess) {
      CudaCheck(cudaEventSynchronize(cb_event.event));
      cb_event.callback();
      cb_event.cuda_stream_handle->PutCudaEvent(cb_event.event);
    }
  });
}

GpuThread::~GpuThread() {
  cb_event_chan_.Close();
  cb_event_poller_.join();
}

#endif

}  // namespace oneflow
