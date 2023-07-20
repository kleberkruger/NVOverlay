#ifndef WRITE_BUFFER_H
#define WRITE_BUFFER_H

#include "write_buffer_entry.h"
#include "subsecond_time.h"
#include "cache_block_info.h"
#include "shmem_perf_model.h"

#include <deque>
#include <map>
#include <variant>
#include <algorithm>

class WriteBuffer
{
public:
   WriteBuffer(UInt32 num_entries) : m_num_entries(num_entries) {}
   virtual ~WriteBuffer() = default;

   virtual void insert(const WriteBufferEntry& entry, SubsecondTime time) = 0;
   virtual WriteBufferEntry remove() = 0;
   // virtual WriteBufferEntry get(IntPtr address) = 0;
   virtual bool isPresent(IntPtr address) = 0;

   SubsecondTime getFirstReleaseTime() { return m_queue.front().second; }
   SubsecondTime getLastReleaseTime() { return m_queue.back().second; }

   bool isEmpty() { return m_queue.empty(); }
   bool isFull() { return m_queue.size() >= m_num_entries; }
   
   UInt32 getSize() { return m_queue.size(); }
   UInt32 getCapacity() { return m_num_entries; }

   // for debug
   void print(String desc = "");
   virtual std::tuple<IntPtr, UInt64, SubsecondTime> 
   getEntryInfo(std::pair<std::variant<WriteBufferEntry, IntPtr>, SubsecondTime>& e) = 0;

protected:
   static const UInt32 DEFAULT_NUMBER_ENTRIES = 32;

   UInt32 m_num_entries;
   std::deque<std::pair<std::variant<WriteBufferEntry, IntPtr>, SubsecondTime>> m_queue;
};


class NonCoalescingWriteBuffer : public WriteBuffer
{
public:

   NonCoalescingWriteBuffer(UInt32 num_entries = DEFAULT_NUMBER_ENTRIES);
   virtual ~NonCoalescingWriteBuffer();

   virtual void insert(const WriteBufferEntry& entry, SubsecondTime time);
   virtual WriteBufferEntry remove();
   // virtual WriteBufferEntry get(IntPtr address);
   virtual bool isPresent(IntPtr address);

   std::tuple<IntPtr, UInt64, SubsecondTime> getEntryInfo(std::pair<std::variant<WriteBufferEntry, IntPtr>, SubsecondTime>& e);
};

class CoalescingWriteBuffer : public WriteBuffer
{
public:

   CoalescingWriteBuffer(UInt32 num_entries = DEFAULT_NUMBER_ENTRIES);
   virtual ~CoalescingWriteBuffer();

   virtual void insert(const WriteBufferEntry& entry, SubsecondTime time);
   virtual void update(const WriteBufferEntry& entry);
   virtual WriteBufferEntry remove();
   // virtual WriteBufferEntry get(IntPtr address);
   virtual bool isPresent(IntPtr address);

   std::tuple<IntPtr, UInt64, SubsecondTime> getEntryInfo(std::pair<std::variant<WriteBufferEntry, IntPtr>, SubsecondTime>& e);

private:
   std::map<IntPtr, WriteBufferEntry> m_map;
};

#endif /* WRITE_BUFFER_H */
