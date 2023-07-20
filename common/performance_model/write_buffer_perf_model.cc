#include "write_buffer_perf_model.h"

WriteBufferPerfModel::WriteBufferPerfModel(UInt32 num_entries, bool coalescing, SubsecondTime insertion_latency)
       : m_num_entries(num_entries),
         m_coalescing(coalescing),
         m_insertion_latency(insertion_latency),
         m_queue(),
         m_total_latency(SubsecondTime::Zero()),
         m_num_accesses(0),
         m_num_insertions(0),
         m_num_overflows(0)
{
   // TODO: Record statistics
   // registerStatsMetric("<cacheName>-writebuffer", core_id, "total-accesses", &m_num_access);
   // registerStatsMetric("<cacheName>-writebuffer", core_id, "total-insertions", &m_num_insertions);
   // registerStatsMetric("<cacheName>-writebuffer", core_id, "total-overflows", &m_num_overflows);
   // registerStatsMetric("<cacheName>-writebuffer", core_id, "total-access-latency", &m_total_access_latency);
   // registerStatsMetric("<cacheName>-writebuffer", core_id, "total-queueing-delay", &m_total_queueing_delay);
}

WriteBufferPerfModel::~WriteBufferPerfModel() = default;

void WriteBufferPerfModel::consome(const SubsecondTime& t_now)
{
   while (!m_queue.empty() && t_now >= m_queue.front().second)
      m_queue.pop_front();
}

void printWriteBuffer(std::deque<std::pair<IntPtr, SubsecondTime>>& queue)
{
   UInt32 index = 0;
   printf("----------------------\n");
   for (auto& e : queue) printf("%4u [%12lX] -> (%lu)\n", index++, e.first, e.second.getNS());
   printf("----------------------\n");
}

SubsecondTime WriteBufferPerfModel::getInsertionLatency(IntPtr address, SubsecondTime send_latency, ShmemPerfModel *perf)
{
   SubsecondTime latency = m_insertion_latency;
   SubsecondTime t_now = perf->getElapsedTime(ShmemPerfModel::_USER_THREAD);
   consome(t_now);

   auto same_address = [&](auto& e) { return e.first == address; };
   if (!m_coalescing || std::find_if(m_queue.rbegin(), m_queue.rend(), same_address) == m_queue.rend())
   {
      SubsecondTime t_start  = m_queue.empty() ? t_now : m_queue.back().second;
      if (m_queue.size() >= m_num_entries)
      {
         latency += m_queue.front().second - t_now;
         m_queue.pop_front();
         m_num_overflows++;
      }   
      m_queue.push_back(std::make_pair(address, t_start + send_latency));
      m_num_insertions++;

      // printWriteBuffer(m_queue);
      // printf("Time: %lu | Written [%lX] in the writebuffer (%luns)\n\n", now.getNS(), address, latency.getNS());
   }
   m_num_accesses++;
   return latency;
}

SubsecondTime WriteBufferPerfModel::flush(ShmemPerfModel *perf)
{
   return m_queue.front().second - perf->getElapsedTime(ShmemPerfModel::_USER_THREAD);
}

SubsecondTime WriteBufferPerfModel::flushAll(ShmemPerfModel *perf)
{
   SubsecondTime latency = SubsecondTime::Zero();
   for (UInt32 i = 0; i < m_queue.size(); i++)
      latency += flush(perf);
   return latency;
}
