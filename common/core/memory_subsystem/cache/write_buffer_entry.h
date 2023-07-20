#ifndef WRITE_BUFFER_ENTRY_H
#define WRITE_BUFFER_ENTRY_H

#include "cache_block_info.h"
#include "shmem_perf_model.h"

class WriteBufferEntry
{
public:

   WriteBufferEntry(IntPtr address, UInt32 offset, Byte* data_buf, UInt32 data_length, 
                    ShmemPerfModel::Thread_t thread_num, UInt64 eid = 0);
   
   WriteBufferEntry(CacheBlockInfo* cache_block);
   
   virtual ~WriteBufferEntry();

   IntPtr getAddress() const { return address; }
   UInt32 getOffset() const { return offset; }
   Byte* getDataBuffer() const { return data_buf; }
   UInt32 getDataLength() const { return data_length; }
   ShmemPerfModel::Thread_t getThreadNum() { return thread_num; }
   UInt64 getEpochID() const { return eid; };

   CacheBlockInfo* getCacheBlockInfo() const { return cache_block; }

// private:

   IntPtr address; // ram
   UInt32 offset;
   Byte* data_buf; // ram
   UInt32 data_length; // ram
   ShmemPerfModel::Thread_t thread_num; // ram
   UInt64 eid;
   CacheBlockInfo* cache_block; // used on send to ram
};

#endif /* WRITE_BUFFER_ENTRY_H */