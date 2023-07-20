#include "write_buffer_entry.h"

WriteBufferEntry::WriteBufferEntry(CacheBlockInfo* cache_block)
    : address(0),
      offset(0),
      data_buf(nullptr),
      data_length(0),
      thread_num(ShmemPerfModel::Thread_t::_USER_THREAD),
      eid(0),
      cache_block(cache_block)
{ }

WriteBufferEntry::WriteBufferEntry(IntPtr address, UInt32 offset, Byte *data_buf, UInt32 data_length,
                                   ShmemPerfModel::Thread_t thread_num, UInt64 eid)
    : address(address),
      offset(offset),
      data_buf(data_buf),
      data_length(data_length),
      thread_num(thread_num),
      eid(eid),
      cache_block(nullptr)
{ }

WriteBufferEntry::~WriteBufferEntry() = default;