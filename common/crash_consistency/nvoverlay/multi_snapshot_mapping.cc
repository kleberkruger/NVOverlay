#include "multi_snapshot_mapping.h"

namespace PrL1PrL2DramDirectoryMSI {

MultiSnapshotNVMMapping* MultiSnapshotNVMMapping::getInstance() 
{
   static MultiSnapshotNVMMapping* instance = new MultiSnapshotNVMMapping();
   return instance;
}

MultiSnapshotNVMMapping::MultiSnapshotNVMMapping() { 
   // for (UInt32 i = 0; i < 4; i++)
   //    m_omc_map.push_back(OverlayMemCntlr());
}

MultiSnapshotNVMMapping::~MultiSnapshotNVMMapping() = default;


void MultiSnapshotNVMMapping::insert(CacheBlockInfo *cache_block_info) 
{ 
   // printf("INSERTING IN MSM address: (%c) %lu\n", cache_block_info->getCStateString(), cache_block_info->getEpochID());
}


}