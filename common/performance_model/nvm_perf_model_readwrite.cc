#include "nvm_perf_model_readwrite.h"
#include "simulator.h"
#include "config.h"
#include "config.hpp"
#include "stats.h"
#include "shmem_perf.h"

NvmPerfModelReadWrite::NvmPerfModelReadWrite(core_id_t core_id, UInt32 cache_block_size) :
   NvmPerfModel(core_id, cache_block_size),
   m_queue_model_read(nullptr),
   m_queue_model_write(nullptr),
   m_nvm_read_cost(NvmPerfModel::getReadLatency()),
   m_nvm_write_cost(NvmPerfModel::getWriteLatency()),
   m_nvm_bandwidth(8 * Sim()->getCfg()->getFloat("perf_model/dram/per_controller_bandwidth")), // Convert bytes to bits
   m_shared_readwrite(Sim()->getCfg()->getBool("perf_model/dram/readwrite/shared")),
   m_total_read_queueing_delay(SubsecondTime::Zero()),
   m_total_write_queueing_delay(SubsecondTime::Zero()),
   m_total_access_latency(SubsecondTime::Zero())
{
   String mem_technology = DramCntlrInterface::getTechnology() == DramCntlrInterface::HYBRID ? "nvm" : "dram";
   if (Sim()->getCfg()->getBool("perf_model/dram/queue_model/enabled")) {
      String queue_model_type = Sim()->getCfg()->getString("perf_model/dram/queue_model/type");
      SubsecondTime rounded_latency = m_nvm_bandwidth.getRoundedLatency(8 * cache_block_size); // bytes to bits

      m_queue_model_read = QueueModel::create(mem_technology + "-queue-read", core_id, queue_model_type, rounded_latency); 
      m_queue_model_write = QueueModel::create(mem_technology + "-queue-write", core_id, queue_model_type, rounded_latency);
   }

   registerStatsMetric(mem_technology, core_id, "total-access-latency", &m_total_access_latency);
   registerStatsMetric(mem_technology, core_id, "total-read-queueing-delay", &m_total_read_queueing_delay);
   registerStatsMetric(mem_technology, core_id, "total-write-queueing-delay", &m_total_write_queueing_delay);
}

NvmPerfModelReadWrite::~NvmPerfModelReadWrite()
{
   if (m_queue_model_read) 
   {
      delete m_queue_model_read;
      m_queue_model_read = nullptr;
      delete m_queue_model_write;
      m_queue_model_write = nullptr;
   }
}

SubsecondTime
NvmPerfModelReadWrite::getAccessLatency(SubsecondTime pkt_time, UInt64 pkt_size, core_id_t requester, IntPtr address,
                                        DramCntlrInterface::access_t access_type, ShmemPerf *perf)
{
   // pkt_size is in 'Bytes'
   // m_dram_bandwidth is in 'Bits per clock cycle'
   if ((!m_enabled) || (requester >= (core_id_t) Config::getSingleton()->getApplicationCores())) {
      return SubsecondTime::Zero();
   }

   SubsecondTime processing_time = m_nvm_bandwidth.getRoundedLatency(8 * pkt_size); // bytes to bits

   // Compute Queue Delay
   SubsecondTime queue_delay;
   if (m_queue_model_read) 
   {
      if (access_type == DramCntlrInterface::READ) 
      {
         queue_delay = m_queue_model_read->computeQueueDelay(pkt_time, processing_time, requester);
         if (m_shared_readwrite) 
         {
            // Shared read-write bandwidth, but reads are prioritized over writes.
            // With fluffy time, where we can't delay a write because of an earlier (in simulated time) read
            // that was simulated later (in wallclock time), we model this in the following way:
            // - reads are only delayed by other reads (through m_queue_model_read), this assumes *all* writes
            //   can be moved out of the way if needed.
            // - writes see contention by both reads and other writes, i.e., m_queue_model_write
            //   is updated on both read and write.
            m_queue_model_write->computeQueueDelay(pkt_time, processing_time, requester);
         }
      } 
      else
         queue_delay = m_queue_model_write->computeQueueDelay(pkt_time, processing_time, requester);
   }
   else 
   {
      queue_delay = SubsecondTime::Zero();
   }

   SubsecondTime access_cost = (access_type == DramCntlrInterface::WRITE) ? m_nvm_write_cost : m_nvm_read_cost;
   SubsecondTime access_latency = queue_delay + processing_time + access_cost;


   perf->updateTime(pkt_time);
   perf->updateTime(pkt_time + queue_delay, ShmemPerf::DRAM_QUEUE);
   perf->updateTime(pkt_time + queue_delay + processing_time, ShmemPerf::DRAM_BUS);
   perf->updateTime(pkt_time + queue_delay + processing_time + access_cost, ShmemPerf::DRAM_DEVICE);

   // Update Memory Counters
   m_num_accesses++;
   m_total_access_latency += access_latency;
   if (access_type == DramCntlrInterface::READ)
      m_total_read_queueing_delay += queue_delay;
   else
      m_total_write_queueing_delay += queue_delay;

   return access_latency;
}
