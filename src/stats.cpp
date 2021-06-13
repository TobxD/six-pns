#ifndef STATS_CPP
#define STATS_CPP
#include "pns.h"
using namespace std;

long long GeneralStats::currentTimeMillis(){
  auto time = chrono::system_clock::now();
  return chrono::duration_cast<chrono::milliseconds>(time.time_since_epoch()).count();
}

void GeneralStats::print(long long totNodes){
  cout << "HT: " << (double)bytesUsedHT / 1e6 << "MB; ";
  cout << "transform: " << (double)bytesUsedTransform / 1e6 << "MB; ";
  cout << "nodes: " << (double)bytesUsedPNS / 1e6 << "MB; ";
  long long sum = bytesUsedHT + bytesUsedTransform + bytesUsedPNS;
  cout << "tot: " << (double)sum / 1e6 << "MB" << "\n";

  long long time = currentTimeMillis() - startTime;
  double time2 = (double)(clock() - startClock) * 1000.0 / CLOCKS_PER_SEC;
  cout << "time: " << time << "ms; ";
  if(totNodes != -1)
    cout << "nodes/s created: " << (double)totNodes * 1000 / (double)time;
  cout << "; time2: " << time2 << "; ";
  if(totNodes != -1)
    cout << "nodes/s created 2: " << (double)totNodes * 1000 / time2;
  cout << "\n";

  cout << "HT: entries: " << hashTableEntries << "; deleted (too full): " << hashTableDeletedTooFull << "\n";
  cout << "isSame: found: " << isSameFound << "; failed: " << isSameFailed << endl;

  cout << "intLev: ";
  for(int i=0; i<40; i++){
    cout << intNodesAtLev[i] << " ";
  }
  cout << "\nchildsLev: ";
  for(int i=0; i<40; i++){
    cout << intNodesChildrenAtLev[i] << " ";
  }
  cout << "\nbranchF: ";
  for(int i=0; i<40; i++){
    cout << (double)intNodesChildrenAtLev[i]/(double)intNodesAtLev[i] << " ";
  }
  cout << "\n";
  cout << endl;
}

void PnsStats::print(){
  double avgTraversedToLeaf = (double)totNodesVisited / (double)totExpanded;
  cout << "numbers at root (expanded " << totExpanded << ") (number of nodes: " << totNodes-totDeleted << ") (totDeleted: " << totDeleted << ") (tot vis: " << totNodesVisited << "/" << avgTraversedToLeaf << "): " << root->mynum << " " << root->othnum << "\n";
#ifdef PN_STATS
  if(root->mynum == 0)
    cout << "real num: " << root->myRealNum << "\n";
  if(root->othnum == 0)
    cout << "real num: " << root->othRealNum << "\n";
#endif
  cout << "levCount: ";
  for(int i=0; i<100; i++){
    if(levCount[i])
      cout << i << ":" << levCount[i] << " ";
  }
  cout << "\n";
  cout << "proved: ";
  for(int i=0; i<100; i++){
    if(proved[i])
      cout << i << ":" << proved[i] << " ";
  }
  cout << "\n";
  cout << "non-proven leaves: ";
  for(int i=0; i<100; i++){
    if(leaves[i])
      cout << i << ":" << leaves[i] << " ";
  }
  cout << "\n";

  generalStats->print(totNodes);
}

#endif
