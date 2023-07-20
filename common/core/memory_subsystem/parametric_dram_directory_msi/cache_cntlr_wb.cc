#include "cache_cntlr_wb.h"

#  define MYLOG(...) {}

namespace ParametricDramDirectoryMSI
{

CacheCntlrWrBuff::CacheCntlrWrBuff(MemComponent::component_t mem_component,
                                   String name,
                                   core_id_t core_id,
                                   MemoryManager *memory_manager,
                                   AddressHomeLookup *tag_directory_home_lookup,
                                   Semaphore *user_thread_sem,
                                   Semaphore *network_thread_sem,
                                   UInt32 cache_block_size,
                                   CacheParameters &cache_params,
                                   ShmemPerfModel *shmem_perf_model,
                                   bool is_last_level_cache) :
      CacheCntlr(mem_component, name, core_id, memory_manager, tag_directory_home_lookup,
                 user_thread_sem, network_thread_sem, cache_block_size, cache_params,
                 shmem_perf_model, is_last_level_cache)
{
   m_writebuffer_cntlr = new WriteBufferCntlr(this, cache_params.configName);
}

CacheCntlrWrBuff::~CacheCntlrWrBuff()
{
   delete m_writebuffer_cntlr;
}

#ifdef ALWAYS_USE_WRITE_BUFFER
void 
CacheCntlrWrBuff::writeCacheBlockToNextLevel(IntPtr address, UInt32 offset, Byte* data_buf, UInt32 data_length, ShmemPerfModel::Thread_t thread_num)
{
   SubsecondTime latency = m_writebuffer_cntlr->insert(address, offset, data_buf, data_length, thread_num);
   getMemoryManager()->incrElapsedTime(latency, ShmemPerfModel::_USER_THREAD);
}
#endif

void 
CacheCntlrWrBuff::writeCacheBlock(IntPtr address, UInt32 offset, Byte* data_buf, UInt32 data_length, ShmemPerfModel::Thread_t thread_num)
{
   MYLOG(" ");

   // TODO: should we update access counter?

   if (m_master->m_evicting_buf && (address == m_master->m_evicting_address)) {
      MYLOG("writing to evict buffer %lx", address);
      assert(offset==0);
      assert(data_length==getCacheBlockSize());
      if (data_buf)
         memcpy(m_master->m_evicting_buf + offset, data_buf, data_length);
   } else {
      __attribute__((unused)) SharedCacheBlockInfo* cache_block_info = (SharedCacheBlockInfo*) m_master->m_cache->accessSingleLine(
         address + offset, Cache::STORE, data_buf, data_length, getShmemPerfModel()->getElapsedTime(thread_num), false);
      LOG_ASSERT_ERROR(cache_block_info, "writethrough expected a hit at next-level cache but got miss");
      LOG_ASSERT_ERROR(cache_block_info->getCState() == CacheState::MODIFIED, "Got writeback for non-MODIFIED line");
   }

   if (m_cache_writethrough) 
   {
      #ifdef PRIVATE_L2_OPTIMIZATION
      acquireStackLock(address, true);
      #endif
      SubsecondTime latency = m_writebuffer_cntlr->insert(address, offset, data_buf, data_length, thread_num);
      getMemoryManager()->incrElapsedTime(latency, ShmemPerfModel::_USER_THREAD);
      #ifdef PRIVATE_L2_OPTIMIZATION
      releaseStackLock(address, true);
      #endif
   }
}

void 
CacheCntlrWrBuff::sendByWriteBuffer(const WriteBufferEntry& entry)
{
   if (!isLastLevel())
   {
      m_next_cache_cntlr->writeCacheBlock(entry.address, entry.offset, entry.data_buf, entry.data_length, entry.thread_num);
      m_next_cache_cntlr->getCacheBlockInfo(entry.address)->setEpochID(entry.eid);
   }
   else 
   {  // TODO: check this method!
      IntPtr address = m_master->m_cache->tagToAddress(entry.cache_block->getTag());
      Byte data_buf[getCacheBlockSize()];
      getMemoryManager()->sendMsg(PrL1PrL2DramDirectoryMSI::ShmemMsg::WB_REP,
                                  MemComponent::LAST_LEVEL_CACHE, MemComponent::TAG_DIR,
                                  m_core_id_master, getHome(address), /* requester and receiver */
                                  address, data_buf, getCacheBlockSize(),
                                  HitWhere::UNKNOWN, &m_dummy_shmem_perf, ShmemPerfModel::_USER_THREAD);
   }
}

}
