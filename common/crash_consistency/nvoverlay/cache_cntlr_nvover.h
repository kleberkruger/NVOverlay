#pragma once

#include "cache_cntlr_wb.h"
#include "epoch_cntlr.h"
#include "multi_snapshot_mapping.h"

namespace ParametricDramDirectoryMSI
{
   
class CacheCntlrNVOver : public CacheCntlrWrBuff
{
public:

   CacheCntlrNVOver(MemComponent::component_t mem_component,
                    String name,
                    core_id_t core_id,
                    MemoryManager *memory_manager,
                    AddressHomeLookup *tag_directory_home_lookup,
                    Semaphore *user_thread_sem,
                    Semaphore *network_thread_sem,
                    UInt32 cache_block_size,
                    CacheParameters &cache_params,
                    ShmemPerfModel *shmem_perf_model,
                    bool is_last_level_cache,
                    EpochCntlr *epoch_cntlr);

   virtual ~CacheCntlrNVOver();

   HitWhere::where_t processMemOpFromCore(Core::lock_signal_t lock_signal,
                                          Core::mem_op_t mem_op_type,
                                          IntPtr ca_address, UInt32 offset,
                                          Byte* data_buf, UInt32 data_length,
                                          bool modeled,
                                          bool count);

   void notifyLogOnLLC(CacheBlockInfo* cache_block_info);

   EpochCntlr* getEpochCntlr() const { return m_epoch_cntlr; }

   void sendByWriteBuffer(const WriteBufferEntry& entry);

private:
   
   EpochCntlr* m_epoch_cntlr;

   SharedCacheBlockInfo* setCacheState(IntPtr address, CacheState::cstate_t cstate);
   void cascadeLogToNextLevels(IntPtr address, UInt32 offset, Byte* data_buf, UInt32 data_length, ShmemPerfModel::Thread_t thread_num);
};

}