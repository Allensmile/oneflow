#ifndef ONEFLOW_CORE_DATASET_DATASET_H_
#define ONEFLOW_CORE_DATASET_DATASET_H_

#include "oneflow/core/data/data_sampler.h"
#include "oneflow/core/common/auto_registration_factory.h"
#include "oneflow/core/data/data.pb.h"
#include "oneflow/core/record/record.pb.h"
#include <memory>

namespace oneflow {
namespace data {

class Dataset {
 public:
  OF_DISALLOW_COPY_AND_MOVE(Dataset);
  Dataset() = delete;
  explicit Dataset(const DatasetProto& proto);
  virtual ~Dataset() = default;

  virtual void Init() {}
  virtual size_t Size() const = 0;
  virtual std::unique_ptr<OFRecord> EncodeOneRecord(int64_t idx) const = 0;
  virtual int64_t GetGroupId(int64_t idx) const { UNIMPLEMENTED(); }
  DataSampler* GetSampler() { return sampler_.get(); }
  void GenDataSequence(int64_t total_data_num);
  std::vector<int64_t> GetPartDataSequence(int64_t part_id, int64_t part_num) const;

  const DatasetProto& dataset_proto() const { return *dataset_proto_; }
  std::unique_ptr<DataSampler>& sampler() { return sampler_; }

 private:
  const DatasetProto* dataset_proto_;
  std::unique_ptr<DataSampler> sampler_;

  std::vector<int64_t> data_seq_;
};

}  // namespace data
}  // namespace oneflow

#define REGISTER_DATASET(k, DerivedDatasetClass) \
  REGISTER_CLASS_WITH_ARGS(k, Dataset, DerivedDatasetClass, const DatasetProto&)
#define REGISTER_DATASET_CREATOR(k, f) REGISTER_CLASS_CREATOR(k, Dataset, f, const DatasetProto&)

#endif  // ONEFLOW_CORE_DATASET_DATASET_H_
