#ifndef OVERLAY_MEM_CNTLR
#define OVERLAY_MEM_CNTLR

#include "nvm_cntlr.h"
#include "cache_block_info.h"
#include "write_buffer_cntlr.h"

namespace PrL1PrL2DramDirectoryMSI {

/**
 * "A single address tagged with different OIDs could be mapped to
 * different physical locations by the Overlay Memory Controller (OMC), 
 * which serves as the memory controller sitting between the cache 
 * hierarchy and the main memory."
 *
 * Overlay Memory Controller (OMC) serves as a memory controller sitting
 * between the cache hierarchy and the main memory to map a single address
 * tagged with different OIDs to different physical locations.
*/
class OverlayMemCntlr : public NvmCntlr {
public:

   OverlayMemCntlr(MemoryManagerBase* memory_manager,
                   ShmemPerfModel* shmem_perf_model,
                   UInt32 cache_block_size);
   virtual ~OverlayMemCntlr();

   inline DramPerfModel* getDramPerfModel();

   void enableDramPerfModel(bool enable);

   boost::tuple<SubsecondTime, HitWhere::where_t> getDataFromDram(IntPtr address, core_id_t requester, Byte* data_buf, SubsecondTime now, ShmemPerf *perf);
   boost::tuple<SubsecondTime, HitWhere::where_t> putDataToDram(IntPtr address, core_id_t requester, Byte* data_buf, SubsecondTime now);

   void handleMsgFromLLC(core_id_t sender, PrL1PrL2DramDirectoryMSI::ShmemMsg* shmem_msg);

   void addCacheBlock(CacheBlockInfo *cacheBlockInfo);         

private:
   UInt64 cur_epoch;
   UInt64 min_ver;
   UInt64 rec_epoch;

   std::map<UInt64, std::vector<CacheBlockInfo*>> m_map;
   
   DramCntlr* m_dram_cntlr;
};

}

#endif /* OVERLAY_MEM_CNTLR */