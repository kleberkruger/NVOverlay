#ifndef DRAM_NVM_CNTLR_H
#define DRAM_NVM_CNTLR_H

#include "dram_cntlr.h"

namespace PrL1PrL2DramDirectoryMSI
{
   class NvmCntlr;

   // TODO: Suggestion to new implementation: inherit from class DramCntlrInterface and compose DramCntlr and NvmCntlr
   class DramNvmCntlr : public DramCntlr
   {
      public:
         DramNvmCntlr(MemoryManagerBase* memory_manager,
               ShmemPerfModel* shmem_perf_model,
               UInt32 cache_block_size);

         virtual ~DramNvmCntlr();
         
         inline DramPerfModel* getNvmPerfModel();

         void enableDramPerfModel(bool enable);

         virtual boost::tuple<SubsecondTime, HitWhere::where_t> getDataFromNvm(IntPtr address, core_id_t requester, Byte* data_buf, SubsecondTime now, ShmemPerf *perf);
         virtual boost::tuple<SubsecondTime, HitWhere::where_t> putDataToNvm(IntPtr address, core_id_t requester, Byte* data_buf, SubsecondTime now);
      
      protected:
         NvmCntlr* m_nvm_cntlr;
   };
}

#endif /* DRAM_NVM_CNTLR_H */
