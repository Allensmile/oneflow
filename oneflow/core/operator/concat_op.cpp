#include "oneflow/core/operator/concat_op.h"

namespace oneflow {

void ConcatOp::InitFromOpConf() {
  CHECK(op_conf().has_concat_conf());

  EnrollRepeatedInputBn("in");
  EnrollOutputBn("out");
}

const PbMessage& ConcatOp::GetCustomizedConf() const { return op_conf().concat_conf(); }

void ConcatOp::InferBlobDescs(std::function<BlobDesc*(const std::string&)> GetBlobDesc4BnInOp,
                              const ParallelContext* parallel_ctx) const {
  const ConcatOpConf& conf = op_conf().concat_conf();
  const BlobDesc* in_0_blob_desc = GetBlobDesc4BnInOp(input_bns().Get(0));
  std::vector<int64_t> out_dim_vec = in_0_blob_desc->shape().dim_vec();
  bool dim0_varing = in_0_blob_desc->has_dim0_valid_num_field();
  int32_t concat_axis = conf.axis();
  if (concat_axis < 0) { concat_axis += out_dim_vec.size(); }
  for (size_t i = 1; i < input_bns().size(); ++i) {
    const BlobDesc* in_i_blob_desc = GetBlobDesc4BnInOp(input_bns().Get(i));
    for (int64_t j = 0; j < in_i_blob_desc->shape().NumAxes(); ++j) {
      if (j == concat_axis) {
        out_dim_vec[j] += in_i_blob_desc->shape().At(j);
      } else {
        CHECK_EQ(out_dim_vec[j], in_i_blob_desc->shape().At(j));
      }
    }
    if (in_i_blob_desc->has_dim0_valid_num_field()) { dim0_varing = true; }
    CHECK_EQ(in_i_blob_desc->data_type(), in_0_blob_desc->data_type());
    CHECK_EQ(in_i_blob_desc->has_data_id_field(), in_0_blob_desc->has_data_id_field());
    CHECK_EQ(in_i_blob_desc->has_instance_shape_field(),
             in_0_blob_desc->has_instance_shape_field());
  }
  BlobDesc* out_blob_desc = GetBlobDesc4BnInOp("out");
  *out_blob_desc = *in_0_blob_desc;
  out_blob_desc->mut_shape() = Shape(out_dim_vec);
  if (dim0_varing) {
    out_blob_desc->set_has_dim0_valid_num_field(true);
    out_blob_desc->mut_dim0_inner_shape() = Shape({1, out_dim_vec.at(0)});
  }
}

REGISTER_OP(OperatorConf::kConcatConf, ConcatOp);

}  // namespace oneflow
