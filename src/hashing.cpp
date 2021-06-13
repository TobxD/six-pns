#ifndef HASHING_CPP
#define HASHING_CPP
#include "pns.h"
using namespace std;

HashingStrategy::HashingStrategy(Board* _b, GeneralStats* _stats): b(_b), table(Hashtable(_stats)), stats(_stats) {
#ifndef HASHING
  return;
#endif
#ifdef MIRROR
  int maxMirror = 2;
#else
  int maxMirror = 1;
#endif
#ifdef ROTATION
  int maxRot = 6;
#else
  int maxRot = 1;
#endif
  for(int mirror=0; mirror<maxMirror; mirror++){
    for(char rot=0; rot<maxRot; rot++){
      hashAlgos.push_back(new PowerHash(PosTransform(0, 0, mirror, rot), _stats));
    }
  }
}

HashingStrategy::~HashingStrategy(){
  for(auto algo : hashAlgos)
    delete(algo);
}

void HashingStrategy::move(Pos& p, char col){
  for(auto algo: hashAlgos)
    algo->move(p, col);
}

void HashingStrategy::unmove(char col){
  for(auto algo: hashAlgos)
    algo->unmove(col);
}

// ! function cannot be used id undefined(HASHING) !
// returns hash value and index of used hash algorithm
pair<powerHashType, int> HashingStrategy::getLookupHash(){
  powerHashType minH = hashAlgos[0]->hashBoard();
  int minHInd = 0;
  powerHashType res = minH;
  for(int i=1; i<(int)hashAlgos.size(); i++){
    auto h = hashAlgos[i]->hashBoard();
    if(h < minH){
      minH = h;
      minHInd = i;
    }
    res ^= h;
  }
  return {res, minHInd};
}

void HashingStrategy::storeCurrentPos(node* x, PosTransform curT){
#ifndef HASHING
  return;
#endif
  powerHashType hashValue;
  int algoIndex;
  tie(hashValue, algoIndex) = getLookupHash();
  PosTransform transform = hashAlgos[algoIndex]->curPosTransform().inv() * curT;
  table.put(hashValue, transform, x);
}

void HashingStrategy::removeCurrentPos(node* x){
#ifndef HASHING
  return;
#endif
  powerHashType hashValue;
  int algoIndex;
  tie(hashValue, algoIndex) = getLookupHash();
  table.remove(hashValue, x);
}

pair<PosTransform, node*> HashingStrategy::getCur(node* parent){
#ifdef HASHING
  powerHashType hashValue;
  int algoIndex;
  tie(hashValue, algoIndex) = getLookupHash();
  auto elem = table.getEntry(hashValue);
  if(elem != NULL){
    PosTransform transform = hashAlgos[algoIndex]->curPosTransform() * elem->transform;
    if(transform.M != 0 && hashAlgos[algoIndex]->isSame(b, elem->treeNode, parent, transform))
      return make_pair(transform, elem->treeNode);
  }
#endif
  PosTransform t(0, 0, false, 0);
  node* n = NULL;
  pair<PosTransform, node*> p = make_pair(t, n);
  return p;
}

#endif
