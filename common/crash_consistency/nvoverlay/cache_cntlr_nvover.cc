#include "cache_cntlr_nvover.h"
#include "memory_manager.h"

#  define MYLOG(...) {}

namespace ParametricDramDirectoryMSI
{

CacheCntlrNVOver::CacheCntlrNVOver(MemComponent::component_t mem_component,
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
                                   EpochCntlr *epoch_cntlr) :
      CacheCntlrWrBuff(mem_component, name, core_id, memory_manager, tag_directory_home_lookup,
                       user_thread_sem, network_thread_sem, cache_block_size, cache_params,
                       shmem_perf_model, is_last_level_cache),
      m_epoch_cntlr(epoch_cntlr)
{
   LOG_ASSERT_ERROR(!m_cache_writethrough, "NVOverlay does not support caches enabled in writethrough mode");
   
   UInt32 cache_levels = (MemComponent::component_t)(Sim()->getCfg()->getInt("perf_model/cache/levels") - 2 + MemComponent::L2_CACHE);
   LOG_ASSERT_ERROR(cache_levels >= 3, "NVOverlay needs at least three cache levels");
}

CacheCntlrNVOver::~CacheCntlrNVOver() = default;

/**
 * @brief Operation called by core on first-level cache
 * 
 * @param lock_signal 
 * @param mem_op_type 
 * @param ca_address 
 * @param offset 
 * @param data_buf 
 * @param data_length 
 * @param modeled 
 * @param count 
 * @return HitWhere::where_t 
 */
HitWhere::where_t 
CacheCntlrNVOver::processMemOpFromCore(Core::lock_signal_t lock_signal,
                                       Core::mem_op_t mem_op_type,
                                       IntPtr ca_address, UInt32 offset,
                                       Byte* data_buf, UInt32 data_length,
                                       bool modeled,
                                       bool count)
{
   if (m_mem_component != MemComponent::component_t::L1_DCACHE)
   {
      return CacheCntlr::processMemOpFromCore(lock_signal, mem_op_type, ca_address, offset, data_buf, data_length, modeled, count);
   }

   HitWhere::where_t hit_where = HitWhere::MISS;

   // Protect against concurrent access from sibling SMT threads
   ScopedLock sl_smt(m_master->m_smt_lock);

   LOG_PRINT("processMemOpFromCore(), lock_signal(%u), mem_op_type(%u), ca_address(0x%x)",
             lock_signal, mem_op_type, ca_address);
   MYLOG("----------------------------------------------");
   MYLOG("%c%c %lx+%u..+%u", mem_op_type == Core::WRITE ? 'W' : 'R', mem_op_type == Core::READ_EX ? 'X' : ' ', ca_address, offset, data_length);
   LOG_ASSERT_ERROR((ca_address & (getCacheBlockSize() - 1)) == 0, "address at cache line + %x", ca_address & (getCacheBlockSize() - 1));
   LOG_ASSERT_ERROR(offset + data_length <= getCacheBlockSize(), "access until %u > %u", offset + data_length, getCacheBlockSize());

   #ifdef PRIVATE_L2_OPTIMIZATION
   /* if this is the second part of an atomic operation: we already have the lock, don't lock again */
   if (lock_signal != Core::UNLOCK)
      acquireLock(ca_address);
   #else
   /* if we'll need the next level (because we're a writethrough cache, and either this is a write
      or we're part of an atomic pair in which this or the other memop is potentially a write):
      make sure to lock it now, so the cache line in L2 doesn't fall from under us
      between operationPermissibleinCache and the writethrough */
   bool lock_all = m_cache_writethrough && ((mem_op_type == Core::WRITE) || (lock_signal != Core::NONE));

    /* if this is the second part of an atomic operation: we already have the lock, don't lock again */
   if (lock_signal != Core::UNLOCK) {
      if (lock_all)
         acquireStackLock(ca_address);
      else
         acquireLock(ca_address);
   }
   #endif

   SubsecondTime t_start = getShmemPerfModel()->getElapsedTime(ShmemPerfModel::_USER_THREAD);

   CacheBlockInfo *cache_block_info;
   bool cache_hit = operationPermissibleinCache(ca_address, mem_op_type, &cache_block_info), prefetch_hit = false;

   if (!cache_hit && m_perfect)
   {
      cache_hit = true;
      hit_where = HitWhere::where_t(m_mem_component);
      if (cache_block_info)
         cache_block_info->setCState(CacheState::MODIFIED);
      else
      {
         insertCacheBlock(ca_address, mem_op_type == Core::READ ? CacheState::SHARED : CacheState::MODIFIED, NULL, m_core_id, ShmemPerfModel::_USER_THREAD);
         cache_block_info = getCacheBlockInfo(ca_address);
      }
   }
   else if (cache_hit && m_passthrough)
   {
      cache_hit = false;
      cache_block_info->invalidate();
      cache_block_info = NULL;
   }

   if (count)
   {
      ScopedLock sl(getLock());
      // Update the Cache Counters
      getCache()->updateCounters(cache_hit);
      updateCounters(mem_op_type, ca_address, cache_hit, getCacheState(cache_block_info), Prefetch::NONE);
   }

   // printf("- %s [%lu] -> %s\n", mem_op_type == Core::READ ? "READ" : mem_op_type == Core::READ_EX ? "READ_EX" : "WRITE", 
   //        ca_address, cache_hit ? "cache hit" : "cache miss");
      
   // Added by Kleber Kruger
   if (mem_op_type == Core::WRITE && getCacheState(cache_block_info) == CacheState::MODIFIED)
      m_epoch_cntlr->newEpoch();

   // if (mem_op_type == Core::WRITE)
   // {
   //    printf("Time: %lu | WRITE [%12lX] (%lu) -> %s\n", getShmemPerfModel()->getElapsedTime(ShmemPerfModel::_USER_THREAD).getNS(),
   //           ca_address, m_epoch_cntlr->getCurrentEID(), cache_hit ? "cache hit" : "cache miss");
   // }

   if (cache_hit)
   {
      MYLOG("L1 hit");
      getMemoryManager()->incrElapsedTime(m_mem_component, CachePerfModel::ACCESS_CACHE_DATA_AND_TAGS, ShmemPerfModel::_USER_THREAD);
      hit_where = (HitWhere::where_t)m_mem_component;

      if (cache_block_info->hasOption(CacheBlockInfo::WARMUP) && Sim()->getInstrumentationMode() != InstMode::CACHE_ONLY)
      {
         stats.hits_warmup++;
         cache_block_info->clearOption(CacheBlockInfo::WARMUP);
      }
      if (cache_block_info->hasOption(CacheBlockInfo::PREFETCH))
      {
         // This line was fetched by the prefetcher and has proven useful
         stats.hits_prefetch++;
         prefetch_hit = true;
         cache_block_info->clearOption(CacheBlockInfo::PREFETCH);
      }

      if (modeled && m_l1_mshr)
      {
         ScopedLock sl(getLock());
         SubsecondTime t_now = getShmemPerfModel()->getElapsedTime(ShmemPerfModel::_USER_THREAD);
         SubsecondTime t_completed = m_master->m_l1_mshr.getTagCompletionTime(ca_address);
         if (t_completed != SubsecondTime::MaxTime() && t_completed > t_now)
         {
            if (mem_op_type == Core::WRITE)
               ++stats.store_overlapping_misses;
            else
               ++stats.load_overlapping_misses;

            SubsecondTime latency = t_completed - t_now;
            getShmemPerfModel()->incrElapsedTime(latency, ShmemPerfModel::_USER_THREAD);
         }
      }

      if (modeled)
      {
         ScopedLock sl(getLock());
         // This is a hit, but maybe the prefetcher filled it at a future time stamp. If so, delay.
         SubsecondTime t_now = getShmemPerfModel()->getElapsedTime(ShmemPerfModel::_USER_THREAD);
         if (m_master->mshr.count(ca_address)
            && (m_master->mshr[ca_address].t_issue < t_now && m_master->mshr[ca_address].t_complete > t_now))
         {
            SubsecondTime latency = m_master->mshr[ca_address].t_complete - t_now;
            stats.mshr_latency += latency;
            getMemoryManager()->incrElapsedTime(latency, ShmemPerfModel::_USER_THREAD);
         }
      }

      // TODO: Mover para cima talvez???
      // Added by Kleber Kruger
      if (mem_op_type == Core::WRITE && cache_block_info->getCState() == CacheState::MODIFIED)
      {
         cascadeLogToNextLevels(ca_address, offset, data_buf, data_length, ShmemPerfModel::_USER_THREAD);
         cache_block_info->setEpochID(m_epoch_cntlr->getCurrentEID());
      }

   } else {
      /* cache miss: either wrong coherency state or not present in the cache */
      MYLOG("L1 miss");
      if (!m_passthrough)
         getMemoryManager()->incrElapsedTime(m_mem_component, CachePerfModel::ACCESS_CACHE_TAGS, ShmemPerfModel::_USER_THREAD);

      SubsecondTime t_miss_begin = getShmemPerfModel()->getElapsedTime(ShmemPerfModel::_USER_THREAD);
      SubsecondTime t_mshr_avail = t_miss_begin;

      if (modeled && m_l1_mshr && !m_passthrough)
      {
         ScopedLock sl(getLock());
         t_mshr_avail = m_master->m_l1_mshr.getStartTime(t_miss_begin);
         LOG_ASSERT_ERROR(t_mshr_avail >= t_miss_begin, "t_mshr_avail < t_miss_begin");
         SubsecondTime mshr_latency = t_mshr_avail - t_miss_begin;
         // Delay until we have an empty slot in the MSHR
         getShmemPerfModel()->incrElapsedTime(mshr_latency, ShmemPerfModel::_USER_THREAD);
         stats.mshr_latency += mshr_latency;
      }

      if (lock_signal == Core::UNLOCK)
         LOG_PRINT_ERROR("Expected to find address(0x%x) in L1 Cache", ca_address);

      #ifdef PRIVATE_L2_OPTIMIZATION
      #else
      if (!lock_all)
         acquireStackLock(ca_address, true);
      #endif

      // Invalidate the cache block before passing the request to L2 Cache
      if (getCacheState(ca_address) != CacheState::INVALID)
      {
         invalidateCacheBlock(ca_address);
      }

      MYLOG("processMemOpFromCore l%d before next", m_mem_component);
      hit_where = m_next_cache_cntlr->processShmemReqFromPrevCache(this, mem_op_type, ca_address, modeled, count, Prefetch::NONE, t_start, false);
      bool next_cache_hit = hit_where != HitWhere::MISS;
      MYLOG("processMemOpFromCore l%d next hit = %d", m_mem_component, next_cache_hit);

      if (next_cache_hit) {

      } else {
         /* last level miss, a message has been sent. */

         MYLOG("processMemOpFromCore l%d waiting for sent message", m_mem_component);
         #ifdef PRIVATE_L2_OPTIMIZATION
         releaseLock(ca_address);
         #else
         releaseStackLock(ca_address);
         #endif

         waitForNetworkThread();
         MYLOG("processMemOpFromCore l%d postwakeup", m_mem_component);

         //acquireStackLock(ca_address);
         // Pass stack lock through from network thread

         wakeUpNetworkThread();
         MYLOG("processMemOpFromCore l%d got message reply", m_mem_component);

         /* have the next cache levels fill themselves with the new data */
         MYLOG("processMemOpFromCore l%d before next fill", m_mem_component);
         hit_where = m_next_cache_cntlr->processShmemReqFromPrevCache(this, mem_op_type, ca_address, false, false, Prefetch::NONE, t_start, true);
         MYLOG("processMemOpFromCore l%d after next fill", m_mem_component);
         LOG_ASSERT_ERROR(hit_where != HitWhere::MISS,
            "Tried to read in next-level cache, but data is already gone");

         #ifdef PRIVATE_L2_OPTIMIZATION
         releaseStackLock(ca_address, true);
         #else
         #endif
      }

      /* data should now be in next-level cache, go get it */
      SubsecondTime t_now = getShmemPerfModel()->getElapsedTime(ShmemPerfModel::_USER_THREAD);
      copyDataFromNextLevel(mem_op_type, ca_address, modeled, t_now);

      cache_block_info = getCacheBlockInfo(ca_address);

      #ifdef PRIVATE_L2_OPTIMIZATION
      #else
      if (!lock_all)
         releaseStackLock(ca_address, true);
      #endif

      LOG_ASSERT_ERROR(operationPermissibleinCache(ca_address, mem_op_type),
         "Expected %x to be valid in L1", ca_address);


      if (modeled && m_l1_mshr && !m_passthrough)
      {
         SubsecondTime t_miss_end = getShmemPerfModel()->getElapsedTime(ShmemPerfModel::_USER_THREAD);
         ScopedLock sl(getLock());
         m_master->m_l1_mshr.getCompletionTime(t_miss_begin, t_miss_end - t_mshr_avail, ca_address);
      }
   }

   // if (mem_op_type == Core::WRITE) printCache(getCache()); // Added by Kleber Kruger

   if (modeled && m_next_cache_cntlr && !m_perfect && Sim()->getConfig()->hasCacheEfficiencyCallbacks())
   {
      bool new_bits = cache_block_info->updateUsage(offset, data_length);
      if (new_bits)
      {
         m_next_cache_cntlr->updateUsageBits(ca_address, cache_block_info->getUsage());
      }
   }

   accessCache(mem_op_type, ca_address, offset, data_buf, data_length, hit_where == HitWhere::where_t(m_mem_component) && count);
   MYLOG("access done");

   SubsecondTime t_now = getShmemPerfModel()->getElapsedTime(ShmemPerfModel::_USER_THREAD);
   SubsecondTime total_latency = t_now - t_start;

   // From here on downwards: not long anymore, only stats update so blanket cntrl lock
   {
      ScopedLock sl(getLock());

      if (! cache_hit && count) {
         stats.total_latency += total_latency;
      }

      #ifdef TRACK_LATENCY_BY_HITWHERE
      if (count)
         lat_by_where[hit_where].update(total_latency.getNS());
      #endif

      /* if this is the first part of an atomic operation: keep the lock(s) */
      #ifdef PRIVATE_L2_OPTIMIZATION
      if (lock_signal != Core::LOCK)
         releaseLock(ca_address);
      #else
      if (lock_signal != Core::LOCK) {
         if (lock_all)
            releaseStackLock(ca_address);
         else
            releaseLock(ca_address);
      }
      #endif

      if (mem_op_type == Core::WRITE)
         stats.stores_where[hit_where]++;
      else
         stats.loads_where[hit_where]++;
   }

   if (modeled && m_master->m_prefetcher)
   {
      trainPrefetcher(ca_address, cache_hit, prefetch_hit, false, t_start);
   }

   // Call Prefetch on next-level caches (but not for atomic instructions as that causes a locking mess)
   if (lock_signal != Core::LOCK && modeled)
   {
      Prefetch(t_start);
   }

   if (Sim()->getConfig()->getCacheEfficiencyCallbacks().notify_access_func)
      Sim()->getConfig()->getCacheEfficiencyCallbacks().call_notify_access(cache_block_info->getOwner(), mem_op_type, hit_where);

   MYLOG("returning %s, latency %lu ns", HitWhereString(hit_where), total_latency.getNS());
   return hit_where;
}

/**
 * @brief Set cache state
 * 
 * @param address 
 * @param cstate 
 * @return SharedCacheBlockInfo* 
 */
SharedCacheBlockInfo* 
CacheCntlrNVOver::setCacheState(IntPtr address, CacheState::cstate_t cstate)
{
   SharedCacheBlockInfo* cache_block_info = getCacheBlockInfo(address);
   cache_block_info->setCState(cstate);

   if (m_mem_component == MemComponent::component_t::L1_DCACHE && cstate == CacheState::MODIFIED)
      cache_block_info->setEpochID(m_epoch_cntlr->getCurrentEID());

   return cache_block_info;
}

/**
 * @brief Send the old L1 cache block to L2 and the L2 cache block to L3
 * 
 * @param ca_address 
 * @param offset 
 * @param data_buf 
 * @param data_length 
 */
void
CacheCntlrNVOver::cascadeLogToNextLevels(IntPtr address, UInt32 offset, Byte* data_buf, UInt32 data_length, ShmemPerfModel::Thread_t thread_num)
{
   if (!m_next_cache_cntlr->isLastLevel())
      static_cast<CacheCntlrNVOver *>(m_next_cache_cntlr)->cascadeLogToNextLevels(address, offset, data_buf, data_length, thread_num);

   #ifdef PRIVATE_L2_OPTIMIZATION
   acquireStackLock(address, true);
   #endif
   
   // printf("Time: %lu | cascadeLog on %s\n", getShmemPerfModel()->getElapsedTime(ShmemPerfModel::_USER_THREAD).getNS(),  getCache()->getName().c_str());
   SubsecondTime latency = m_writebuffer_cntlr->insert(address, offset, data_buf, data_length, thread_num, 
                                                       getCacheBlockInfo(address)->getEpochID());
   getShmemPerfModel()->incrElapsedTime(latency, ShmemPerfModel::_USER_THREAD);

   #ifdef PRIVATE_L2_OPTIMIZATION
   releaseStackLock(address, true);
   #endif
}

void 
CacheCntlrNVOver::notifyLogOnLLC(CacheBlockInfo* cache_block_info)
{
   // getMemoryManager()->getMultiSnapMap()->insert(cache_block_info);
   m_writebuffer_cntlr->insert(WriteBufferEntry(cache_block_info));
}

void 
CacheCntlrNVOver::sendByWriteBuffer(const WriteBufferEntry& entry)
{
   if (!isLastLevel())
   {
      m_next_cache_cntlr->writeCacheBlock(entry.address, entry.offset, entry.data_buf, entry.data_length, entry.thread_num);
      m_next_cache_cntlr->getCacheBlockInfo(entry.address)->setEpochID(entry.eid);

      if (m_next_cache_cntlr->m_mem_component == MemComponent::component_t::L3_CACHE)
         static_cast<CacheCntlrNVOver*>(m_next_cache_cntlr)->notifyLogOnLLC(m_next_cache_cntlr->getCacheBlockInfo(entry.address));
   }
   else 
   {
      IntPtr address = m_master->m_cache->tagToAddress(entry.cache_block->getTag());
      Byte data_buf[getCacheBlockSize()];
      getMemoryManager()->sendMsg(PrL1PrL2DramDirectoryMSI::ShmemMsg::NVM_LOG_REQ,
                                  MemComponent::LAST_LEVEL_CACHE, MemComponent::NVM,
                                  m_core_id_master, getHome(address), /* requester and receiver */
                                  address, data_buf, getCacheBlockSize(),
                                  HitWhere::UNKNOWN, &m_dummy_shmem_perf, ShmemPerfModel::_USER_THREAD);
   }
}

}
