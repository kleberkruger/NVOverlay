#include "dram_nvm_cntlr.h"
#include "nvm_cntlr.h"

namespace PrL1PrL2DramDirectoryMSI
{

DramNvmCntlr::DramNvmCntlr(MemoryManagerBase* memory_manager,
      ShmemPerfModel* shmem_perf_model,
      UInt32 cache_block_size)
      : DramCntlr(memory_manager, shmem_perf_model, cache_block_size)
{
    m_nvm_cntlr = new NvmCntlr(memory_manager, shmem_perf_model, cache_block_size);
}

DramNvmCntlr::~DramNvmCntlr()
{
   delete m_nvm_cntlr;
}

boost::tuple<SubsecondTime, HitWhere::where_t>
DramNvmCntlr::getDataFromNvm(IntPtr address, core_id_t requester, Byte* data_buf, SubsecondTime now, ShmemPerf *perf)
{
   return m_nvm_cntlr->getDataFromNvm(address, requester, data_buf, now, perf);
}

boost::tuple<SubsecondTime, HitWhere::where_t>
DramNvmCntlr::putDataToNvm(IntPtr address, core_id_t requester, Byte* data_buf, SubsecondTime now)
{
   return m_nvm_cntlr->putDataToNvm(address, requester, data_buf, now);
}

void
DramNvmCntlr::enableDramPerfModel(bool enable)
{
   DramCntlr::enableDramPerfModel(enable);
   m_nvm_cntlr->enableNvmPerfModel(enable);
}

DramPerfModel* 
DramNvmCntlr::getNvmPerfModel() 
{ 
    return m_nvm_cntlr->getNvmPerfModel(); 
}

// TODO: if not ENABLE_DRAM_ACCESS_COUNT, this method is not used!
// void DramNvmCntlr::addToDramAccessCount(IntPtr address, DramCntlrInterface::access_t access_type) {
//    m_dram_access_count[access_type][address] = m_dram_access_count[access_type][address] + 1;
// }

}
