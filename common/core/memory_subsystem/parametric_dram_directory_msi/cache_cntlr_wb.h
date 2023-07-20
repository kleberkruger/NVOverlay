#pragma once

#include "cache_cntlr.h"
#include "write_buffer_cntlr.h"
#include "memory_manager.h"
#include "config.hpp"

// #define ALWAYS_USE_WRITE_BUFFER

namespace ParametricDramDirectoryMSI
{

class CacheCntlrWrBuff : public CacheCntlr
{
public:

   CacheCntlrWrBuff(MemComponent::component_t mem_component,
                    String name,
                    core_id_t core_id,
                    MemoryManager *memory_manager,
                    AddressHomeLookup *tag_directory_home_lookup,
                    Semaphore *user_thread_sem,
                    Semaphore *network_thread_sem,
                    UInt32 cache_block_size,
                    CacheParameters &cache_params,
                    ShmemPerfModel *shmem_perf_model,
                    bool is_last_level_cache);

   virtual ~CacheCntlrWrBuff();

   #ifdef ALWAYS_USE_WRITE_BUFFER
   virtual void writeCacheBlockToNextLevel(IntPtr address, UInt32 offset, Byte* data_buf, UInt32 data_length, ShmemPerfModel::Thread_t thread_num);
   #endif

   virtual void writeCacheBlock(IntPtr address, UInt32 offset, Byte* data_buf, UInt32 data_length, ShmemPerfModel::Thread_t thread_num);

   friend class WriteBufferCntlr;

protected:

   WriteBufferCntlr* m_writebuffer_cntlr;

   virtual void sendByWriteBuffer(const WriteBufferEntry& entry);
};

}