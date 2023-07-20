// #include "dram_cntlr_interface.h"

// namespace PrL1PrL2DramDirectoryMSI
// {
//    class DramCntlr;
//    class NvmCntlr;

//    class DramNvmCntlr : public DramCntlrInterface
//    {
//       protected:
//          DramCntlr* m_dram_cntlr;
//          NvmCntlr* m_nvm_cntlr;

//       public:
//          DramNvmCntlr(MemoryManagerBase* memory_manager,
//                       ShmemPerfModel* shmem_perf_model,
//                       UInt32 cache_block_size);

//          virtual ~DramNvmCntlr();
         
//          DramPerfModel* getDramPerfModel() { return m_dram_cntlr->getDramPerfModel(); }
//          DramPerfModel* getNvmPerfModel() { return m_nvm_cntlr->getNvmPerfModel(); }

//          // Run DRAM performance model. Pass in begin time, returns latency
//          virtual boost::tuple<SubsecondTime, HitWhere::where_t> getDataFromDram(IntPtr address, core_id_t requester, Byte* data_buf, SubsecondTime now, ShmemPerf *perf);
//          virtual boost::tuple<SubsecondTime, HitWhere::where_t> putDataToDram(IntPtr address, core_id_t requester, Byte* data_buf, SubsecondTime now);

//          virtual boost::tuple<SubsecondTime, HitWhere::where_t> getDataFromNvm(IntPtr address, core_id_t requester, Byte* data_buf, SubsecondTime now, ShmemPerf *perf);
//          virtual boost::tuple<SubsecondTime, HitWhere::where_t> putDataToNvm(IntPtr address, core_id_t requester, Byte* data_buf, SubsecondTime now);

//          virtual void printDramAccessCount(void);
         
//          virtual void enableDramPerfModel(bool enable);
//    };
// }
