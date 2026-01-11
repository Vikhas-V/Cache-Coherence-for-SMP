#include <stdlib.h>
#include <assert.h>
#include <iostream>
#include <stdio.h>
#include "cache.h"
#include "bus_ops.h"
#include <iomanip>
#include <fstream>

using namespace std;

Cache::~Cache()
{
   // Safely delete the 2D cache array
   if(cache != NULL) {
       for(ulong i = 0; i < sets; i++) {
           if(cache[i] != NULL) {
               delete[] cache[i];
           }
       }
       delete[] cache;
   }
}

Cache::Cache(int s, int a, int b)
{
   ulong i, j;
   reads = readMisses = writes = 0;
   writeMisses = writeBacks = currentCycle = 0;
   mem_trans = interventions = invalidations = flushes = busrdX = cache2cache = 0;

   size       = (ulong)(s);
   lineSize   = (ulong)(b);
   assoc      = (ulong)(a);
   sets       = (ulong)((s/b)/a);
   numLines   = (ulong)(s/b);
   log2Sets   = (ulong)(log2(sets));
   log2Blk    = (ulong)(log2(b));

   tagMask = 0;
   for(i = 0; i < log2Sets; i++)
   {
       tagMask <<= 1;
       tagMask |= 1;
   }

   cache = NULL;
   cache = new cacheLine*[sets];
   for(i = 0; i < sets; i++)
   {
      cache[i] = NULL;
      cache[i] = new cacheLine[assoc];
      for(j = 0; j < assoc; j++)
      {
         cache[i][j].invalidate();
      }
   }
}


void Cache::Access(ulong addr, uchar op, uint protocol, int proc, vector<Cache*>& v)
{
   currentCycle++;
    int num_proc = v.size();

   if(op == 'w')
        writes++;
   else
        reads++;

   cacheLine* line = findLine(addr);

   if(line == NULL)//MISS
   {
       cacheLine* newline = fillLine(addr);

        if(op == 'w') // WRITEMISS
        {
            writeMisses++;
            switch(protocol)
            {
                case 0:
                    busrdX++;
                    busRdX(addr, proc, num_proc, protocol, v);
                    newline->setFlag(MODIFIED);
                    break;

                case 1:
                    if (copyExists(addr, proc, num_proc, v))
                        cache2cache++;
                    busrdX++;
                    busRdX(addr, proc, num_proc, protocol, v);
                    newline->setFlag(MODIFIED);
                    break;
                case 2:
                    if (copyExists(addr, proc, num_proc, v))
                    {
                        newline->setFlag(SHARED_MOD);
                        busRd(addr, proc, num_proc, protocol, v);
                        busUpdate(addr, proc, num_proc, v);
                    }
                    else
                    {
                        newline->setFlag(MODIFIED);
                        busRd(addr, proc, num_proc, protocol, v);
                    }
                    break;
            }
        }
       else          // READMISS
        {
            readMisses++;
            switch (protocol)
            {
                case 0:
                    newline->setFlag(SHARED);
                    break;

                case 1:
                    if(copyExists(addr, proc, num_proc, v))
                    {
                        cache2cache++;
                        newline->setFlag(SHARED);
                    }
                    else
                        newline->setFlag(EXCLUSIVE);
                    break;

                case 2:
                    if (copyExists(addr, proc, num_proc, v))
                        newline->setFlag(SHARED);
                    else
                        newline->setFlag(EXCLUSIVE);
                    break;
            }
            busRd(addr, proc, num_proc, protocol, v);
        }
    }

   else //HIT
   {
       
       updateLRU(line);
       if(op == 'w') //WRITE HIT
        {
            ulong flag = line->getFlag();
            switch (protocol)
            {
                case 0:
                    if (flag == SHARED)
                    {
                        line->setFlag(MODIFIED);
                        mem_trans++;
                        busrdX++;
                        busRdX(addr, proc, num_proc, protocol, v);
                    }
                    break;

                case 1:
                    if (flag == SHARED)
                    {
                        line->setFlag(MODIFIED);
                        busUpgr(addr, proc, num_proc, protocol, v);
                    }
                    else if (flag == EXCLUSIVE)
                        line->setFlag(MODIFIED);
                    break;

                case 2:
                    if(flag == SHARED)
                    {
                        busUpdate(addr, proc, num_proc, v);
                        if (copyExists(addr, proc, num_proc, v))
                            line->setFlag(SHARED_MOD);
                        else
                            line->setFlag(MODIFIED);
                    }
                    else if (flag == SHARED_MOD)
                    {
                        busUpdate(addr, proc, num_proc, v);
                        if (!copyExists(addr, proc, num_proc, v))
                            line->setFlag(MODIFIED);
                    }
                    else if (flag == EXCLUSIVE)
                        line->setFlag(MODIFIED);
            }

        }
   }
}


cacheLine* Cache::findLine(ulong addr)
{
   ulong i, j, tag, pos;
   pos = assoc;
   tag = calcTag(addr);
   i   = calcIndex(addr);

   for(j = 0; j < assoc; j++)
   {
      if(cache[i][j].isValid())
        if(cache[i][j].getTag() == tag)
       {
            pos = j; 
            break;
       }
   }

   if(pos == assoc)
    return NULL;
   else
    return &(cache[i][pos]);
}

void Cache::updateLRU(cacheLine* line)
{
  line->setSeq(currentCycle);
}

cacheLine* Cache::getLRU(ulong addr)
{
   ulong i, j, victim, min;

   victim = assoc;
   min    = currentCycle;
   i      = calcIndex(addr);

   for(j = 0; j < assoc; j++)
   {
      if(cache[i][j].isValid() == 0)
        return &(cache[i][j]);
   }
   for(j = 0; j < assoc; j++)
   {
     if(cache[i][j].getSeq() <= min)
     {
        victim = j;
        min = cache[i][j].getSeq();
     }
   }
   assert(victim != assoc);

   return &(cache[i][victim]);
}

cacheLine* Cache::findLineToReplace(ulong addr)
{
   cacheLine* victim = getLRU(addr);
   updateLRU(victim);

   return (victim);
}

cacheLine* Cache::fillLine(ulong addr)
{
   ulong tag;
   cacheLine* victim = findLineToReplace(addr);

   assert(victim != 0);
   if(victim->getFlag() == MODIFIED || victim->getFlag() == SHARED_MOD)
      writeBack(addr);

   tag = calcTag(addr);
   victim->setTag(tag);
   victim->setFlag(INVALID);

   return victim;
}

void Cache::printStats(int i, int protocol)
{
   
   cout<<"============ Simulation results (Cache "<<i<<") ============\n";
   cout<<" 01. number of reads:      "<<reads<<"\n 02. number of read misses:      "<<readMisses;
    cout<<"\n 03. number of writes:      "<<writes<<"\n 04. number of write misses:      "<<writeMisses;
    cout<<"\n 05. total miss rate:      "<<fixed <<setprecision(2) <<(float)((readMisses+writeMisses)*100)/(reads+writes)<<"%";
    cout<<"\n 06. number of writebacks:      "<<writeBacks;
    cout<<"\n 07. number of cache-to-cache transfers:     "<<cache2cache;
    if (protocol == 0)
        cout<<"\n 08. number of memory transactions:       "<<readMisses+writeMisses+writeBacks+mem_trans;
    else
        cout<<"\n 08. number of memory transactions:       "<<readMisses+writeMisses+writeBacks-cache2cache;
    cout<<"\n 09. number of interventions:      "<<interventions;
    cout<<"\n 10. number of invalidations:      "<<invalidations;
    cout<<"\n 11. number of flushes:      "<<flushes;
    cout<<"\n 12. number of BusRdX:      "<<busrdX<<endl;

}
