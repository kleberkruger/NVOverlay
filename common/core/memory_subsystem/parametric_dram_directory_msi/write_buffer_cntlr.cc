#include "write_buffer_cntlr.h"
#include "cache_cntlr_wb.h"
#include "nvm_perf_model.h"

namespace ParametricDramDirectoryMSI
{

WriteBufferCntlr::WriteBufferCntlr(CacheCntlrWrBuff* cache_cntlr, String cache_name) 
    : WriteBufferCntlr(cache_cntlr, getNumEntries(cache_name),
                                    getInsertionLatency(cache_name),
                                    isCoalescing(cache_name))
{ }

WriteBufferCntlr::WriteBufferCntlr(CacheCntlrWrBuff* cache_cntlr, UInt32 num_entries, 
                                   SubsecondTime insertion_latency, bool coalescing)
    : m_buffer(nullptr),
      m_perf_model(nullptr),
      m_cache_cntlr(cache_cntlr),
      m_insertion_latency(insertion_latency),
      m_coalescing(coalescing)
{
   if (Sim()->getCfg()->getBoolDefault("perf_model/writebuffer/async_model", false))
   {
      m_buffer = m_coalescing ? (WriteBuffer *) new CoalescingWriteBuffer(num_entries) : 
                                (WriteBuffer *) new NonCoalescingWriteBuffer(num_entries);
   }
   else
   {
      m_perf_model = new WriteBufferPerfModel(num_entries, coalescing, insertion_latency);
   }
}

WriteBufferCntlr::~WriteBufferCntlr()
{
   if (m_buffer)     delete m_buffer;
   if (m_perf_model) delete m_perf_model;
}

SubsecondTime WriteBufferCntlr::insert(IntPtr address, UInt32 offset, Byte* data_buf, UInt32 data_length, 
                                       ShmemPerfModel::Thread_t thread_num, UInt64 eid)
{
   return insert(WriteBufferEntry(address, offset, data_buf, data_length, thread_num, eid));
}

SubsecondTime WriteBufferCntlr::insert(const WriteBufferEntry& entry)
{
   if (isAsynchronous())
   {
      SubsecondTime latency = m_insertion_latency;
      SubsecondTime t_now = m_cache_cntlr->getShmemPerfModel()->getElapsedTime(ShmemPerfModel::_USER_THREAD);

      if (m_coalescing && m_buffer->isPresent(entry.address))
         static_cast<CoalescingWriteBuffer *>(m_buffer)->update(entry);
      else
      {
         SubsecondTime t_start  = m_buffer->isEmpty() ? t_now : m_buffer->getLastReleaseTime();
         SubsecondTime send_latency = getSendLatency(m_cache_cntlr);
         if (m_buffer->isFull())
            latency += flush();
         m_buffer->insert(entry, t_start + send_latency);
         // // for debug
         // static_cast<CoalescingWriteBuffer *>(m_buffer)->print(m_cache_cntlr->getCache()->getName());
         // printf("Time: %lu | Written [%lX] in the writebuffer (%luns)\n", t_now.getNS(), entry.address, latency.getNS());
      }
      return latency;
   }
   else 
   {
      send(entry);
      return m_perf_model->getInsertionLatency(entry.address, getSendLatency(m_cache_cntlr), m_cache_cntlr->getShmemPerfModel());
   }
}

SubsecondTime WriteBufferCntlr::flush()
{
   if (isAsynchronous())
   {
      SubsecondTime t_now = m_cache_cntlr->getShmemPerfModel()->getElapsedTime(ShmemPerfModel::_USER_THREAD);
      SubsecondTime latency = m_buffer->getFirstReleaseTime() - t_now;
      send(m_buffer->remove());
      return latency;
   }
   else
   {
      return m_perf_model->flush(m_cache_cntlr->getShmemPerfModel());
   }
}

SubsecondTime WriteBufferCntlr::flushAll()
{
   if (isAsynchronous())
   {
      SubsecondTime latency = SubsecondTime::Zero();
      for (UInt32 i = 0; i < m_buffer->getSize(); i++)
         latency += flush();
      return latency;
   }
   else
   {
      return m_perf_model->flushAll(m_cache_cntlr->getShmemPerfModel());
   }
}

void WriteBufferCntlr::send(const WriteBufferEntry& entry)
{
   m_cache_cntlr->sendByWriteBuffer(entry);
}

UInt32 
WriteBufferCntlr::getNumEntries(const String& cache_name)
{
   const String key = "perf_model/" + cache_name + "/writebuffer/num_entries";
   return Sim()->getCfg()->hasKey(key) ? Sim()->getCfg()->getInt(key) : UINT32_MAX;
}

SubsecondTime 
WriteBufferCntlr::getInsertionLatency(const String& cache_name)
{
   const String key = "perf_model/" + cache_name + "/writebuffer/insertion_latency";
   return Sim()->getCfg()->hasKey(key) ? SubsecondTime::NS(Sim()->getCfg()->getInt(key)) : SubsecondTime::Zero();
}

SubsecondTime 
WriteBufferCntlr::getSendLatency(CacheCntlrWrBuff* cache_cntlr)
{
   if (!cache_cntlr->isLastLevel()) 
   {
      return cache_cntlr->getMemoryManager()->getCost(cache_cntlr->m_next_cache_cntlr->m_mem_component,
                                                   CachePerfModel::ACCESS_CACHE_DATA_AND_TAGS);
   }

   static SubsecondTime mem_latency = SubsecondTime::Zero();
   if (mem_latency == SubsecondTime::Zero())
   {
      String technology = Sim()->getCfg()->hasKey("perf_model/dram/technology") && Sim()->getCfg()->getString("perf_model/dram/technology") == "nvm" ? "nvm" : "dram";
      mem_latency = technology == "nvm" ? NvmPerfModel::getWriteLatency()
                                        : SubsecondTime::FS() * static_cast<uint64_t>(TimeConverter<float>::NStoFS(Sim()->getCfg()->getFloat("perf_model/dram/latency")));
   }
   return mem_latency;
}

bool
WriteBufferCntlr::isCoalescing(const String& cache_name)
{
   const String key = "perf_model/" + cache_name + "/writebuffer/coalescing";
   return Sim()->getCfg()->getBoolDefault(key, true);
}

}
