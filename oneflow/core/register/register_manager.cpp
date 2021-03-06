/*
Copyright 2020 The OneFlow Authors. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#include "oneflow/core/register/register_manager.h"
#include "oneflow/core/job/job_desc.h"
#include "oneflow/core/register/blob.h"
#include "oneflow/core/common/str_util.h"
#include "oneflow/core/common/tensor_buffer.h"
#include "oneflow/core/comm_network/comm_network.h"
#include "oneflow/core/job/machine_context.h"
#include "oneflow/core/memory/memory_case.pb.h"
#include "oneflow/core/memory/memory_allocator.h"

namespace oneflow {

namespace {

void CheckBlobInRegstNotDisabled(const RegstDescProto& regst_desc) {
  CHECK(regst_desc.regst_desc_type().has_data_regst_desc());
  CHECK(regst_desc.regst_desc_type().data_regst_desc().packed_blob_desc().is_body_disabled()
        == false);
}

}  // namespace

RegstMgr::RegstMgr(const Plan& plan) {
  int64_t this_machine_id = Global<MachineCtx>::Get()->this_machine_id();
  HashMap<int64_t, char*> chunk_id2ptr;
  for (const ChunkProto& chunk : plan.block_chunk_list().chunk()) {
    if (chunk.machine_id() != this_machine_id) { continue; }
    if (chunk.mem_size() == 0) { continue; }
    char* chunk_ptr = Global<MemoryAllocator>::Get()->Allocate(chunk.mem_case(), chunk.mem_size());
    CHECK(chunk_id2ptr.emplace(chunk.chunk_id(), chunk_ptr).second);
  }
  for (const MemBlockProto& mem_block : plan.block_chunk_list().mem_block()) {
    if (mem_block.machine_id() != this_machine_id) { continue; }
    if (mem_block.mem_size() == 0) { continue; }
    char* mem_block_ptr = nullptr;
    if (mem_block.has_chunk_id()) {
      CHECK(mem_block.has_chunk_offset());
      CHECK(chunk_id2ptr.find(mem_block.chunk_id()) != chunk_id2ptr.end());
      mem_block_ptr = chunk_id2ptr.at(mem_block.chunk_id()) + mem_block.chunk_offset();
    } else {
      mem_block_ptr =
          Global<MemoryAllocator>::Get()->Allocate(mem_block.mem_case(), mem_block.mem_size());
    }
    CHECK(mem_block_id2ptr_.emplace(mem_block.mem_block_id(), mem_block_ptr).second);
  }
  for (const TaskProto& task : plan.task()) {
    if (task.machine_id() != this_machine_id) { continue; }
    for (const auto& pair : task.produced_regst_desc()) {
      const RegstDescProto& regst_desc = pair.second;
      CHECK(
          regst_desc_id2rt_regst_desc_
              .emplace(regst_desc.regst_desc_id(), std::make_unique<const RtRegstDesc>(regst_desc))
              .second);
    }
  }
}

void RegstMgr::NewRegsts(const RegstDescProto& regst_desc_proto,
                         std::function<void(Regst*)> OneRegstDone) {
  const int64_t regst_desc_id = regst_desc_proto.regst_desc_id();
  const RegstDescTypeProto& regst_desc_type = regst_desc_proto.regst_desc_type();
  const RtRegstDesc* rt_regst_desc = regst_desc_id2rt_regst_desc_.at(regst_desc_id).get();
  char* main_mem_ptr = nullptr;
  char* separated_header_mem_ptr = nullptr;
  int64_t mem_block_id = regst_desc_proto.mem_block_id();
  int64_t header_block_id = regst_desc_proto.separated_header_mem_block_id();
  if (mem_block_id != -1 && mem_block_id2ptr_.find(mem_block_id) != mem_block_id2ptr_.end()) {
    main_mem_ptr = mem_block_id2ptr_.at(mem_block_id) + regst_desc_proto.mem_block_offset();
  }
  if (header_block_id != -1 && mem_block_id2ptr_.find(header_block_id) != mem_block_id2ptr_.end()) {
    separated_header_mem_ptr = mem_block_id2ptr_.at(header_block_id);
  }
  std::vector<LbiBlobDescPair> lbi_pairs;
  if (regst_desc_type.has_data_regst_desc()) {
    for (const LbiBlobDescPair& pair : regst_desc_type.data_regst_desc().lbi2blob_desc()) {
      lbi_pairs.push_back(pair);
    }
    std::sort(lbi_pairs.begin(), lbi_pairs.end(), &CompareLbiBlobDescPair);
    CHECK(!lbi_pairs.empty());
  }
  for (int64_t i = 0; i < rt_regst_desc->register_num(); ++i) {
    Regst* regst = new Regst;
    regst->set_regst_desc(rt_regst_desc);
    if (regst_desc_type.has_data_regst_desc()) {
      NewBlobsInOneRegst(lbi_pairs, regst, rt_regst_desc, main_mem_ptr, separated_header_mem_ptr);
      if (rt_regst_desc->mem_case().has_host_mem()
          && rt_regst_desc->mem_case().host_mem().used_by_network()) {
        CheckBlobInRegstNotDisabled(regst_desc_proto);
        regst->comm_net_token_ = Global<CommNet>::Get()->RegisterMemory(
            main_mem_ptr, rt_regst_desc->MainByteSize4OneRegst());
      }
      if (main_mem_ptr != nullptr) { main_mem_ptr += rt_regst_desc->MainByteSize4OneRegst(); }
      if (separated_header_mem_ptr != nullptr) {
        separated_header_mem_ptr += rt_regst_desc->SeparatedHeaderByteSize4OneRegst();
      }
    } else if (regst_desc_type.has_ctrl_regst_desc()) {
      // do nothing
    } else {
      UNIMPLEMENTED();
    }
    OneRegstDone(regst);
  }
}

void RegstMgr::NewBlobsInOneRegst(const std::vector<LbiBlobDescPair>& lbis, Regst* regst,
                                  const RtRegstDesc* rt_regst_desc, char* main_mem_ptr,
                                  char* separated_header_mem_ptr) {
  size_t separated_header_mem_size = rt_regst_desc->SeparatedHeaderByteSize4OneRegst();
  const RtBlobDesc* packed_blob_desc = rt_regst_desc->packed_blob_desc();
  char* cur_body_pointer = nullptr;
  char* cur_header_pointer = nullptr;
  if (separated_header_mem_size > 0) {
    MemoryCase host_mem_case;
    host_mem_case.mutable_host_mem();
    if (separated_header_mem_ptr == nullptr) {
      separated_header_mem_ptr =
          Global<MemoryAllocator>::Get()->Allocate(host_mem_case, separated_header_mem_size);
    }
    regst->packed_blob_.reset(new Blob(regst->regst_desc()->mem_case(), packed_blob_desc,
                                       separated_header_mem_ptr, main_mem_ptr));
    cur_header_pointer = separated_header_mem_ptr;
    cur_body_pointer = main_mem_ptr;
  } else {
    CHECK(separated_header_mem_ptr == nullptr);
    regst->packed_blob_.reset(
        new Blob(regst->regst_desc()->mem_case(), packed_blob_desc, main_mem_ptr));
    cur_header_pointer = main_mem_ptr;
    if (main_mem_ptr == nullptr || packed_blob_desc->is_body_disabled()) {
      cur_body_pointer = nullptr;
    } else {
      cur_body_pointer = main_mem_ptr + packed_blob_desc->ByteSizeOfBlobHeader();
    }
  }
  rt_regst_desc->ForEachBlobDescOffsetInOnRegst(
      lbis, [&](const LbiBlobDescPair& lbi, int64_t body_offset, int64_t header_offset) {
        const RtBlobDesc* blob_desc = rt_regst_desc->GetRtBlobDescFromLbi(lbi.lbi());
        std::unique_ptr<Blob> blob_ptr;
        if (cur_body_pointer == nullptr) {
          CHECK(rt_regst_desc->is_body_disabled());
          blob_ptr = std::move(std::make_unique<Blob>(regst->regst_desc()->mem_case(), blob_desc,
                                                      cur_header_pointer + header_offset, nullptr));
        } else {
          CHECK(rt_regst_desc->is_body_disabled() == false);
          blob_ptr = std::move(std::make_unique<Blob>(regst->regst_desc()->mem_case(), blob_desc,
                                                      cur_header_pointer + header_offset,
                                                      cur_body_pointer + body_offset));
          InitNonPODTypeBlobIfNeed(Global<MemoryAllocator>::Get(), blob_ptr.get());
        }
        CHECK(regst->lbi2blob_.emplace(lbi.lbi(), std::move(blob_ptr)).second);
      });
}

const RtRegstDesc& RegstMgr::RegstDesc4RegstDescId(int64_t regst_desc_id) const {
  const auto& it = regst_desc_id2rt_regst_desc_.find(regst_desc_id);
  CHECK(it != regst_desc_id2rt_regst_desc_.end());
  return *it->second;
}

}  // namespace oneflow
