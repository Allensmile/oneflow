#ifndef ONEFLOW_CORE_DATASET_DATASET_H_
#define ONEFLOW_CORE_DATASET_DATASET_H_

#include "oneflow/core/common/auto_registration_factory.h"
#include "oneflow/core/data/data.pb.h"
#include "oneflow/core/record/record.pb.h"
#include <memory>

namespace oneflow {
namespace data {

// class DatasetStatus final {
// public:
//   OF_DISALLOW_COPY_AND_MOVE(DatasetStatus);
//   DatasetStatus() : epoch_(-1), prepared_(false) {}
//   // int64_t ForwardEpoch(int64_t epoch) { return epoch - epoch_.load(); }
//   // void GenDataSequence(int64_t dataset_size, int64_t epoch, bool shuffle);

// private:
//   std::vector<int64_t> data_seq_;
//   std::atomic<int64_t> epoch_;
//   std::atomic<bool> prepared_;
//   std::mutex mtx_;
//   std::condition_variable cond_var_;
// };

class Dataset {
 public:
  OF_DISALLOW_COPY_AND_MOVE(Dataset);
  Dataset() = delete;
  explicit Dataset(const DatasetProto& proto);
  virtual ~Dataset() = default;

  virtual void Init() {}
  virtual size_t Size() const = 0;
  virtual std::unique_ptr<OFRecord> EncodeOneRecord(int64_t idx) const = 0;
  // void GenNewEpochDataSequence(int64_t epoch);
  void GenDataSequence(int64_t total_data_num);
  std::vector<int64_t> GetPartDataSequence(int64_t part_id, int64_t part_num) const;

  const DatasetProto& dataset_proto() const { return *dataset_proto_; }

 private:
  const DatasetProto* dataset_proto_;
  std::vector<int64_t> data_seq_;
  // std::unique_ptr<DatasetStatus> status_;
};

}  // namespace data
}  // namespace oneflow

#define REGISTER_DATASET(k, DerivedDatasetClass) \
  REGISTER_CLASS_WITH_ARGS(k, Dataset, DerivedDatasetClass, const DatasetProto&)
#define REGISTER_DATASET_CREATOR(k, f) REGISTER_CLASS_CREATOR(k, Dataset, f, const DatasetProto&)

#endif  // ONEFLOW_CORE_DATASET_DATASET_H_
