#ifndef POWER_HASH
#define POWER_HASH
#include "pns.h"
using namespace std;

const long long MOD = 1e9 + 7;

//random primes
const long long py[2] = {684903403, 395415197};
const long long px[2] = {234810943, 313566263};
long long invPy[2], invPx[2];
long long powerX[2][2*BOARD_SIZE], powerY[2][2*BOARD_SIZE];

const int offset = 2*BOARD_SIZE;

long long modPow(long long base, long long exp){
  if(exp == 0) return 1;
  long long res = modPow(base, exp/2);
  res = res * res % MOD;
  if(exp%2)
    res = res * base % MOD;
  return res;
}

// using fermat
long long modInv(long long x){
  return modPow(x, MOD-2);
}

void initPower(){
  if(powerX[0][0] != 0)
    return;
  for(int i=0; i<2; i++){
    invPy[i] = modInv(py[i]);
    invPx[i] = modInv(px[i]);
    powerX[i][0] = powerY[i][0] = 1;
    for(int j=1; j<100; j++){
      powerX[i][j] = (powerX[i][j-1]*px[i]) % MOD;
      powerY[i][j] = (powerY[i][j-1]*py[i]) % MOD;
    }
  }
}

PowerHash::PowerHash(PosTransform _toBoard, GeneralStats* _stats)
    : stats(_stats), toBoard(_toBoard), fromBoard(_toBoard.inv()) {
  for(int i=0; i<4*BOARD_SIZE; i++){
    cntX[i] = cntY[i] = 0;
  }
  initPower();
}

void PowerHash::move(Pos& p, char col){
  int colIndex = (int)col - 1;
  Pos newp = fromBoard.transform(p);
  moves.push(newp);

  int x = newp.x + offset;
  int y = newp.y + offset;

  // extend bounding box in y-direction
  if(y < ymin){
    for(int i=0; i<2; i++)
      (curHash[i] *= powerY[i][ymin-y]) %= MOD;
    ymin = y;
  }
  cntY[y]++;

  // extend bounding box in x-direction
  if(x < xmin){
    for(int i=0; i<2; i++)
      (curHash[i] *= powerX[i][xmin-x]) %= MOD;
    xmin = x;
  }
  cntX[x]++;

  int yOffset = y - ymin;
  int xOffset = x - xmin;
  // add current new token
  (curHash[colIndex] += powerX[colIndex][xOffset] * powerY[colIndex][yOffset]) %= MOD;
}

void PowerHash::unmove(char col){
  int colIndex = (int)col - 1;
  auto newp = moves.top();
  moves.pop();

  int x = newp.x + offset;
  int y = newp.y + offset;

  int yOffset = y - ymin;
  int xOffset = x - xmin;
  // undo move
  (curHash[colIndex] += MOD - powerX[colIndex][xOffset] * powerY[colIndex][yOffset] % MOD) %= MOD;

  cntY[y]--;
  cntX[x]--;

  // cut bounding box in y-direction
  if(y == ymin){
    while(cntY[ymin] == 0 && ymin < 2*offset)
      ymin++;
    int ydiff = ymin-y;
    for(int k=0; k<ydiff; k++)
      for(int i=0; i<2; i++)
        (curHash[i] *= invPy[i]) %= MOD;
  }

  // cut bounding box in x-direction
  if(x == xmin){
    while(cntX[xmin] == 0 && xmin < 2*offset)
      xmin++;
    int xdiff = xmin-x;
    for(int k=0; k<xdiff; k++)
      for(int i=0; i<2; i++)
        (curHash[i] *= invPx[i]) %= MOD;
  }
}

powerHashType PowerHash::hashBoard(){
  return curHash[0] + curHash[1];
}

PosTransform PowerHash::curPosTransform(){
  PosTransform transform(xmin-offset, ymin-offset, false, 0);
  return toBoard * transform;
}

/*
assumed: no duplicate moves to leaf
check two things:
- every move leading to LCA is done in curBoard
- same number of moves, i.e., they are on the same level in the tree
 */
bool PowerHash::isSame(Board* curBoard, node* leaf, node* curParent, PosTransform othToMe){
  node* cur = leaf;
  int curCol = curBoard->lastMoveCol();
  bool same = false;
  while(!cur->parents.empty()){
    cur = cur->parents[0];
    bool foundCorrectChild = false;
    for(auto& child : cur->childs){
      if(child.nxt == leaf){
        othToMe = othToMe * child.transform.inv();
        if(curBoard->getPos(othToMe.transform(child.move)) != curCol){
          goto RET;
        }
        foundCorrectChild = true;
        break;
      }
    }
    assert(foundCorrectChild);
    curCol = 3-curCol;
    leaf = cur;
    if(cur == curParent && cur->parents.empty()){
      same = true;
      break;
    }
    if(curParent->parents.empty())
      break;
    curParent = curParent->parents[0];
  }
RET:
  if(same){
    for(auto startMove : curBoard->startMoves){
      if(curBoard->getPos(othToMe.transform(startMove.first)) != startMove.second){
        same = false;
        break;
      }
    }
  }
  if(same){
    stats->isSameFound++;
  }
  else{
    stats->isSameFailed++;
  }
  return same;
}

#endif
