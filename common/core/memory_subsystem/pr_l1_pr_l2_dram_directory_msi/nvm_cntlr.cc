#include "nvm_cntlr.h"
#include "nvm_perf_model.h"
#include "log.h"

namespace PrL1PrL2DramDirectoryMSI
{

NvmCntlr::NvmCntlr(MemoryManagerBase* memory_manager,
      ShmemPerfModel* shmem_perf_model,
      UInt32 cache_block_size)
   : DramCntlr(memory_manager, shmem_perf_model, cache_block_size,
               NvmPerfModel::createNvmPerfModel(memory_manager->getCore()->getId(), cache_block_size),
               MemComponent::NVM)
{ }

NvmCntlr::~NvmCntlr() = default;

boost::tuple<SubsecondTime, HitWhere::where_t> 
NvmCntlr::getDataFromNvm(IntPtr address, core_id_t requester, Byte* data_buf, SubsecondTime now, ShmemPerf *perf)
{
   return DramCntlr::getDataFromDram(address, requester, data_buf, now, perf);
}

boost::tuple<SubsecondTime, HitWhere::where_t> 
NvmCntlr::putDataToNvm(IntPtr address, core_id_t requester, Byte* data_buf, SubsecondTime now)
{
   return DramCntlr::putDataToDram(address, requester, data_buf, now);
}

void
NvmCntlr::printDramAccessCount()
{
   for (UInt32 k = 0; k < DramCntlrInterface::NUM_ACCESS_TYPES; k++)
   {
      for (AccessCountMap::iterator i = m_dram_access_count[k].begin(); i != m_dram_access_count[k].end(); i++)
      {
         if ((*i).second > 100)
         {
            LOG_PRINT("Nvm Cntlr(%i), Address(0x%x), Access Count(%llu), Access Type(%s)",
                  m_memory_manager->getCore()->getId(), (*i).first, (*i).second,
                  (k == READ)? "READ" : (k == WRITE)? "READ" : "LOG");
         }
      }
   }
}

}