#include "overlay_mem_cntlr.h"
#include "log.h"

namespace PrL1PrL2DramDirectoryMSI {

OverlayMemCntlr::OverlayMemCntlr(MemoryManagerBase* memory_manager,
                                 ShmemPerfModel* shmem_perf_model,
                                 UInt32 cache_block_size) 
      : NvmCntlr(memory_manager, shmem_perf_model, cache_block_size),
        m_dram_cntlr(nullptr)
{
   auto technology = DramCntlrInterface::getTechnology();
   LOG_ASSERT_ERROR(technology != DramCntlrInterface::DRAM, "Main Memory (RAM) must be set to NVM or hybrid")
   if (technology == DramCntlrInterface::HYBRID) {
      m_dram_cntlr = new DramCntlr(memory_manager, shmem_perf_model, cache_block_size);
   }
}

OverlayMemCntlr::~OverlayMemCntlr()
{
   if (m_dram_cntlr) delete m_dram_cntlr;
}

boost::tuple<SubsecondTime, HitWhere::where_t>
OverlayMemCntlr::getDataFromDram(IntPtr address, core_id_t requester, Byte* data_buf, SubsecondTime now, ShmemPerf *perf)
{
   if (m_dram_cntlr)
      return m_dram_cntlr->getDataFromDram(address, requester, data_buf, now, perf);

   return NvmCntlr::getDataFromDram(address, requester, data_buf, now, perf);
}

boost::tuple<SubsecondTime, HitWhere::where_t>
OverlayMemCntlr::putDataToDram(IntPtr address, core_id_t requester, Byte* data_buf, SubsecondTime now)
{
   if (m_dram_cntlr)
      return m_dram_cntlr->putDataToDram(address, requester, data_buf, now);

   return NvmCntlr::putDataToDram(address, requester, data_buf, now);
}

void
OverlayMemCntlr::enableDramPerfModel(bool enable)
{
   NvmCntlr::enableDramPerfModel(enable);
   if (m_dram_cntlr) m_dram_cntlr->enableDramPerfModel(enable);
}

DramPerfModel* 
OverlayMemCntlr::getDramPerfModel() 
{ 
    return m_dram_cntlr->getDramPerfModel(); 
}

void OverlayMemCntlr::handleMsgFromLLC(core_id_t sender, PrL1PrL2DramDirectoryMSI::ShmemMsg* shmem_msg)
{
   PrL1PrL2DramDirectoryMSI::ShmemMsg::msg_t shmem_msg_type = shmem_msg->getMsgType();
   SubsecondTime msg_time = getShmemPerfModel()->getElapsedTime(ShmemPerfModel::_SIM_THREAD);
   shmem_msg->getPerf()->updateTime(msg_time);

   switch (shmem_msg_type)
   {
      // case PrL1PrL2DramDirectoryMSI::ShmemMsg::DRAM_READ_REQ:
      // {
      //    IntPtr address = shmem_msg->getAddress();
      //    Byte data_buf[getCacheBlockSize()];
      //    SubsecondTime dram_latency;
      //    HitWhere::where_t hit_where;

      //    boost::tie(dram_latency, hit_where) = getDataFromDram(address, shmem_msg->getRequester(), data_buf, msg_time, shmem_msg->getPerf());

      //    getShmemPerfModel()->incrElapsedTime(dram_latency, ShmemPerfModel::_SIM_THREAD);

      //    shmem_msg->getPerf()->updateTime(getShmemPerfModel()->getElapsedTime(ShmemPerfModel::_SIM_THREAD),
      //       hit_where == HitWhere::DRAM_CACHE ? ShmemPerf::DRAM_CACHE : ShmemPerf::DRAM);

      //    getMemoryManager()->sendMsg(PrL1PrL2DramDirectoryMSI::ShmemMsg::DRAM_READ_REP,
      //          MemComponent::NVM, MemComponent::LAST_LEVEL_CACHE,
      //          shmem_msg->getRequester() /* requester */,
      //          sender /* receiver */,
      //          address,
      //          data_buf, getCacheBlockSize(),
      //          hit_where, shmem_msg->getPerf(), ShmemPerfModel::_SIM_THREAD);
      //    break;
      // }

      // case PrL1PrL2DramDirectoryMSI::ShmemMsg::DRAM_WRITE_REQ:
      // {
      //    putDataToDram(shmem_msg->getAddress(), shmem_msg->getRequester(), shmem_msg->getDataBuf(), msg_time);

      //    // DRAM latency is ignored on write

      //    break;
      // }

      case PrL1PrL2DramDirectoryMSI::ShmemMsg::NVM_LOG_REQ:
      {
         // putDataToNvm(shmem_msg->getAddress(), shmem_msg->getRequester(), shmem_msg->getDataBuf(), msg_time);
         // SubsecondTime nvm_latency;
         // HitWhere::where_t hit_where;
         // boost::tie(nvm_latency, hit_where) = putDataToNvm(shmem_msg->getAddress(), shmem_msg->getRequester(), shmem_msg->getDataBuf(), msg_time);
         // printf("Adicionando latencia de logging: %lu\n", nvm_latency.getNS());
         // getShmemPerfModel()->incrElapsedTime((nvm_latency * 0.1), ShmemPerfModel::_SIM_THREAD);
         break;
      }

      default:
         LOG_PRINT_ERROR("Unrecognized Shmem Msg Type: %u", shmem_msg_type);
         break;
   }
}

}