#include "write_buffer.h"

void WriteBuffer::print(String desc)
{
   UInt32 index = 0;
   if (desc != "") printf("*** %s ***\n", desc.c_str());
   printf("----------------------\n");
   for (auto& e : m_queue) 
   {
      auto tp = getEntryInfo(e);
      printf("%4u [%12lX] (%lu) -> (%lu)\n", index++, std::get<0>(tp), std::get<1>(tp), std::get<2>(tp).getNS());
   }
   printf("----------------------\n");
}


NonCoalescingWriteBuffer::NonCoalescingWriteBuffer(UInt32 num_entries) 
    : WriteBuffer(num_entries) { }

NonCoalescingWriteBuffer::~NonCoalescingWriteBuffer() = default;

void NonCoalescingWriteBuffer::insert(const WriteBufferEntry& entry, SubsecondTime time)
{
   m_queue.push_back(std::make_pair(entry, time));
}

WriteBufferEntry NonCoalescingWriteBuffer::remove()
{
   WriteBufferEntry entry = std::get<WriteBufferEntry>(m_queue.front().first);
   m_queue.pop_front();
   
   return entry;
}

bool NonCoalescingWriteBuffer::isPresent(IntPtr address)
{
   auto same_address = [&](auto& e) { return std::get<WriteBufferEntry>(e.first).address == address; };
   return std::find_if(m_queue.begin(), m_queue.end(), same_address) != m_queue.end();
}

std::tuple<IntPtr, UInt64, SubsecondTime> 
NonCoalescingWriteBuffer::getEntryInfo(std::pair<std::variant<WriteBufferEntry, IntPtr>, SubsecondTime>& e)
{
   auto entry = std::get<WriteBufferEntry>(e.first);
   return std::make_tuple(entry.address, entry.eid, e.second);
}


CoalescingWriteBuffer::CoalescingWriteBuffer(UInt32 num_entries) 
    : WriteBuffer(num_entries), m_map() 
{ }

CoalescingWriteBuffer::~CoalescingWriteBuffer() = default;

void CoalescingWriteBuffer::insert(const WriteBufferEntry& entry, SubsecondTime time)
{
   m_queue.push_back(std::make_pair(entry.address, time));
   m_map.insert(std::make_pair(entry.address, entry));
}

void CoalescingWriteBuffer::update(const WriteBufferEntry& entry)
{
   m_map.find(entry.address)->second = entry;
}

WriteBufferEntry CoalescingWriteBuffer::remove()
{
   WriteBufferEntry entry = m_map.at(std::get<IntPtr>(m_queue.front().first));
   m_map.erase(entry.address);
   m_queue.pop_front();

   return entry;
}

bool CoalescingWriteBuffer::isPresent(IntPtr address)
{
   return m_map.count(address) > 0;
}

std::tuple<IntPtr, UInt64, SubsecondTime> 
CoalescingWriteBuffer::getEntryInfo(std::pair<std::variant<WriteBufferEntry, IntPtr>, SubsecondTime>& e)
{
   auto entry = m_map.at(std::get<IntPtr>(e.first));
   return std::make_tuple(entry.address, entry.eid, e.second);
}
