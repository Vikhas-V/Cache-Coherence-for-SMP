#include "bus_ops.h"
#include <vector>

using namespace std;

void busRd(ulong addr, int proc, int num_proc, int protocol, vector<Cache*>& caches)
{
    for(int i = 0; i < num_proc; i++)
    {
        if (i != proc)
        {
            cacheLine* line_exists = caches[i]->findLine(addr);
            if(line_exists != NULL)
            {
                ulong flag = line_exists->getFlag();
                if (protocol != 2)
                {
                    if (flag == MODIFIED || flag == EXCLUSIVE)
                    {
                        line_exists->setFlag(SHARED);
                        caches[i]->interventions++;

                        if (flag == MODIFIED)
                        {
                            caches[i]->writeBack(addr);
                            caches[i]->flushes++;
                        }
                    }
                }
                else
                {
                    if (flag == EXCLUSIVE)
                    {
                        line_exists->setFlag(SHARED);
                        caches[i]->interventions++;
                    }
                    else if (flag == MODIFIED)
                    {
                        line_exists->setFlag(SHARED_MOD);
                        caches[i]->interventions++;
                        caches[i]->flushes++;
                    }
                    else if (flag == SHARED_MOD)
                        caches[i]->flushes++;
                }
            }
        }
    }
}

void busRdX(ulong addr, int proc, int num_proc, int protocol, vector<Cache*>& caches)
{
    for(int i = 0; i < num_proc; i++)
    {
        if (i != proc)
        {
            cacheLine* line_exists = caches[i]->findLine(addr);
            if(line_exists != NULL)
            {
                ulong flag = line_exists->getFlag();

                if (flag == MODIFIED)
                {
                    caches[i]->writeBack(addr);
                    line_exists->setFlag(INVALID);
                    caches[i]->invalidations++;
                    caches[i]->flushes++;
                }
                else if (flag == SHARED || flag == EXCLUSIVE)
                {
                    line_exists->setFlag(INVALID);
                    caches[i]->invalidations++;
                }
            }
        }
    }
}

void busUpgr(ulong addr, int proc, int num_proc, int protocol, vector<Cache*>& caches)
{
    for(int i = 0; i < num_proc; i++)
    {
        if (i != proc)
        {
            cacheLine* line_exists = caches[i]->findLine(addr);
            if(line_exists != NULL)
            {
                ulong flag = line_exists->getFlag();
                if (flag == SHARED)
                {
                    line_exists->setFlag(INVALID);
                    caches[i]->invalidations++;
                }
            }
        }
    }
}

void busUpdate(ulong addr, int proc, int num_proc, vector<Cache*>& caches)
{
    for(int i = 0; i < num_proc; i++)
    {
        if (i != proc)
        {
            cacheLine* line_exists = caches[i]->findLine(addr);
            if(line_exists != NULL)
                if (line_exists->getFlag() == SHARED_MOD)
                    line_exists->setFlag(SHARED);
        }
    }
}

bool copyExists(ulong addr, int proc, int num_proc, vector<Cache*>& caches)
{
    for(int i = 0; i < num_proc; i++)
    {
        if (i != proc)
        {
            cacheLine* line_exists = caches[i]->findLine(addr);
            if(line_exists != NULL)
                return true;
        }
    }
    return false;
}
