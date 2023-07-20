#ifndef MULTI_SNAPSHOT_MAPPING
#define MULTI_SNAPSHOT_MAPPING

#include "cache_block_info.h"
#include "overlay_mem_cntlr.h"

#include <vector>

namespace PrL1PrL2DramDirectoryMSI {

class MultiSnapshotNVMMapping {
public:

   virtual ~MultiSnapshotNVMMapping();
   
   void insert(CacheBlockInfo *cache_block);

   void putDataToDram(CacheBlockInfo *cache_block);

   void putDataToNvm(CacheBlockInfo *cache_block);

   static MultiSnapshotNVMMapping* getInstance();

private:

   // std::vector<OverlayMemCntlr> m_omc_map;

   MultiSnapshotNVMMapping();
};

}

#endif /* MULTI_SNAPSHOT_MAPPING */