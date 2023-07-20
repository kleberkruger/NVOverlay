#ifndef CACHE_STATE_H
#define CACHE_STATE_H

#include <cassert>

#include "fixed_types.h"

class CacheState
{
   public:
      enum cstate_t
      {
         CSTATE_FIRST = 0,
         INVALID = CSTATE_FIRST,
         SHARED,
         SHARED_UPGRADING,
         EXCLUSIVE,
         OWNED,
         MODIFIED,
         NUM_CSTATE_STATES,
         /* Below are special states, used only for reporting */
         INVALID_COLD = NUM_CSTATE_STATES,
         INVALID_EVICT,
         INVALID_COHERENCY,
         NUM_CSTATE_SPECIAL_STATES
      };

      CacheState(cstate_t state = INVALID) : cstate(state) {}
      ~CacheState() {}

      bool readable()
      {
         return (cstate == MODIFIED) || (cstate == OWNED) || (cstate == SHARED) || (cstate == EXCLUSIVE);
      }

      bool writable()
      {
         return (cstate == MODIFIED);
      }

      char c_str() // Added by Kleber Kruger
      {
         switch(cstate)
         {
            case CacheState::INVALID:           return 'I';
            case CacheState::SHARED:            return 'S';
            case CacheState::SHARED_UPGRADING:  return 'u';
            case CacheState::MODIFIED:          return 'M';
            case CacheState::EXCLUSIVE:         return 'E';
            case CacheState::OWNED:             return 'O';
            case CacheState::INVALID_COLD:      return '_';
            case CacheState::INVALID_EVICT:     return 'e';
            case CacheState::INVALID_COHERENCY: return 'c';
            default:                            return '?';
         }
      }

   private:
      cstate_t cstate;

};

#endif /* CACHE_STATE_H */
