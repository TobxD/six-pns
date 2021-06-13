#ifndef BASIC_HASHTABLE
#define BASIC_HASHTABLE
#include "pns.h"

Hashtable::Hashtable(GeneralStats* _stats): stats(_stats){
  table = new HashtableEntry[SIZE];
}

Hashtable::~Hashtable(){
  delete[] table;
}

HashtableEntry* Hashtable::getEntry(long long hash){
  long long index = (hash & INDEX_MASK)*ASSOCIATIVE_FACTOR;
  for(int i=0; i<ASSOCIATIVE_FACTOR; i++){
    if(table[index+i].fullHash == hash && table[index+i].lastUsedTime != 0){
      table[index+i].lastUsedTime = timeStamp;
      return &table[index+i]; 
    }
  }
  return NULL;
}

// only if free spot left
void Hashtable::put(long long hash, PosTransform transform, node* treeNode){
  long long index = (hash & INDEX_MASK)*ASSOCIATIVE_FACTOR;
  int minTimeIndex = 0;
  int minTime = 2e9; // infinity
  for(int i=0; i<ASSOCIATIVE_FACTOR && minTime > 0; i++){
    if(table[index+i].lastUsedTime < minTime){
      minTime = table[index+i].lastUsedTime;
      minTimeIndex = i;
    }
  }
  if(minTime == 0)
    stats->hashTableEntries++;
  else
    stats->hashTableDeletedTooFull++;
  table[index+minTimeIndex].fullHash = hash;
  table[index+minTimeIndex].transform = transform;
  table[index+minTimeIndex].treeNode = treeNode;
  timeStampCnt2++;
  if(timeStampCnt2 == timeSlowdownFactor){
    timeStampCnt2 = 0;
    timeStamp++;
  }
  table[index+minTimeIndex].lastUsedTime = timeStamp;
}

void Hashtable::remove(long long hash, node* treeNode){
  long long index = (hash & INDEX_MASK)*ASSOCIATIVE_FACTOR;
  for(int i=0; i<ASSOCIATIVE_FACTOR; i++){
    if(table[index+i].fullHash == hash && table[index+i].treeNode == treeNode){
      table[index+i].fullHash = -1;
      table[index+i].lastUsedTime = 0;
      stats->hashTableEntries--;
      return;
    }
  }
}

#endif
