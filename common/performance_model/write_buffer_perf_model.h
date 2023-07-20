#ifndef __WRITE_BUFFER_PERF_MODEL_H__
#define __WRITE_BUFFER_PERF_MODEL_H__

#include "subsecond_time.h"
#include "shmem_perf_model.h"

#include <deque>
#include <algorithm>

class WriteBufferPerfModel
{
public:

   WriteBufferPerfModel(UInt32 num_entries, bool coalescing = true, 
                        SubsecondTime insertion_latency = SubsecondTime::Zero());
   virtual ~WriteBufferPerfModel();

   SubsecondTime getInsertionLatency(IntPtr address, SubsecondTime send_latency, ShmemPerfModel *perf);

   SubsecondTime flush(ShmemPerfModel *perf);
   SubsecondTime flushAll(ShmemPerfModel *perf);

   // Statistics
   SubsecondTime getAvgLatency() { return m_total_latency / m_num_insertions; }
   UInt64 getTotalAccesses() { return m_num_accesses; }
   UInt64 getTotalInsertions() { return m_num_insertions; }
   UInt64 getTotalOverflows() { return m_num_overflows; }

private:

   UInt32 m_num_entries;
   bool m_coalescing;
   SubsecondTime m_insertion_latency;
   std::deque<std::pair<IntPtr, SubsecondTime>> m_queue;

   // Statistics
   SubsecondTime m_total_latency;
   UInt64 m_num_accesses;
   UInt64 m_num_insertions;
   UInt64 m_num_overflows;

   void consome(const SubsecondTime& now);
};

#endif /* __WRITE_BUFFER_PERF_MODEL_H__ */
