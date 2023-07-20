#ifndef NVM_CNTLR_H
#define NVM_CNTLR_H

#include "dram_cntlr.h"

namespace PrL1PrL2DramDirectoryMSI
{
   class NvmCntlr : public DramCntlr
   {
      public:
         NvmCntlr(MemoryManagerBase* memory_manager,
               ShmemPerfModel* shmem_perf_model,
               UInt32 cache_block_size);

         virtual ~NvmCntlr();

         // These methods below are similar to the base class
         DramPerfModel* getNvmPerfModel() { return m_dram_perf_model; }
         void enableNvmPerfModel(bool enable) { return DramCntlr::enableDramPerfModel(enable); }

         virtual boost::tuple<SubsecondTime, HitWhere::where_t> getDataFromNvm(IntPtr address, core_id_t requester, Byte* data_buf, SubsecondTime now, ShmemPerf *perf);
         virtual boost::tuple<SubsecondTime, HitWhere::where_t> putDataToNvm(IntPtr address, core_id_t requester, Byte* data_buf, SubsecondTime now);

      protected:
         virtual void printDramAccessCount(void);
   };
}

#endif /* NVM_CNTLR_H */