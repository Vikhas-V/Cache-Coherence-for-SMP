#ifndef BUS_OPS_H
#define BUS_OPS_H

#include "cache.h"
#include <vector>

using namespace std;

// Bus read operation
void busRd(ulong addr, int proc, int num_proc, int protocol, vector<Cache*>& caches);

// Bus read exclusive operation
void busRdX(ulong addr, int proc, int num_proc, int protocol, vector<Cache*>& caches);

// Bus upgrade operation
void busUpgr(ulong addr, int proc, int num_proc, int protocol, vector<Cache*>& caches);

// Bus update operation
void busUpdate(ulong addr, int proc, int num_proc, vector<Cache*>& caches);

// Check if copy exists in other caches
bool copyExists(ulong addr, int proc, int num_proc, vector<Cache*>& caches);

#endif
