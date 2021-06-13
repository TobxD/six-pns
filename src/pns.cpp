#ifndef PNS_CPP
#define PNS_CPP
#include "pns.h"
using namespace std;

const int MAX_HASH_LEV = 100; // infinity
int DRAW_DEPTH;
int DRAW_WIN_COLOR;

node::node(Pos& move, Board* b, char col, int lev, PnsStats& stats){
  stats.totNodes++;
  char othCol = (char) (3-col);
  char winCol = 0; // 0 -> noone has won
  if(b->hasWon(move)) winCol = othCol;
  else if(lev == stats.drawDepth) winCol = col;
  else if(b->finishedCircles[col-1][5] || b->finishedTriangles[col-1][5] || b->finishedLines[col-1][LINE_LENGTH-1]) winCol = col;
  else if(stats.drawDepth-lev < 8){
    // stop early if no one can win within move limit
    int minMissing = 6;
    int noDrawWinCol = 2-DRAW_WIN_COLOR;
    for(int i=1; i<6; i++){
      if(b->finishedCircles[noDrawWinCol][6-i] || b->finishedTriangles[noDrawWinCol][6-i] || b->finishedLines[noDrawWinCol][LINE_LENGTH-i]){
        minMissing = i;
        break;
      }
    }
    if((stats.drawDepth-lev+(col == DRAW_WIN_COLOR ? 0 : 1))/2 < minMissing)
      winCol = (char)DRAW_WIN_COLOR;
  }

  if(winCol == othCol){
    mynum = INF_PN+1;
    othnum = 0;
#ifdef PN_STATS
    othRealNum = 1;
#endif
  }
  else if(winCol == col){
    mynum = 0;
    othnum = INF_PN+1;
#ifdef PN_STATS
    myRealNum = 1;
#endif
  }
  else{
    stats.leaves[lev]++;
#if defined(HEURISTIC_VALUES)
    mynum = calcNum(b, col, true, lev);
    othnum = calcNum(b, col, false, lev);
#else
    mynum = othnum = 1;
#endif
  }

  stats.generalStats->bytesUsedPNS += sizeof(node) + 36; //empirically determined approximation because of memory overhead when allocating with new and vectors
}

long long node::calcNum(Board* b, char col, bool toMove, int lev){
  int myColInd = col-1, othColInd = 1-(col-1);
  if(!toMove)
    swap(myColInd, othColInd);
  vector<int> me(7), oth(7);
  for(int i=0; i<6; i++){
    me[6-i] += b->finishedCircles[myColInd][6-i];
    me[6-i] += b->finishedLines[myColInd][LINE_LENGTH-i]; //line length could be 5: in that case, count it as i+1 to keep it consistent
    me[6-i] += b->finishedTriangles[myColInd][6-i];
    oth[6-i] += b->finishedCircles[othColInd][6-i];
    oth[6-i] += b->finishedLines[othColInd][LINE_LENGTH-i];
    oth[6-i] += b->finishedTriangles[othColInd][6-i];
  }

  int branchFactor = (int) b->getPosNextMoves().size();
  int notToMoveFactor = toMove ? 1 : branchFactor;

  int res = branchFactor*branchFactor*branchFactor*notToMoveFactor;
  // approximation of proof numbers using completeness of winning shapes
  if(me[5] >= 2) res = 1*notToMoveFactor;
  else if(me[5]) res = branchFactor*notToMoveFactor;
  else if(me[4]) res = max(5, 30-10*me[4])*branchFactor*notToMoveFactor;

  if((col == DRAW_WIN_COLOR) == toMove && lev > DRAW_DEPTH-6){
    bool drawTurn = col == DRAW_WIN_COLOR;
    int pw = (DRAW_DEPTH-lev+drawTurn)/2;
    if(pw == 2)
      res = min(res, branchFactor*branchFactor);
    else if(pw == 1)
      res = min(res, branchFactor);
  }

  return res;
}

int clearMarks(node* cur){
  int res = cur->lev2;
  cur->lev2 = false;
  for(auto c : cur->childs){
    res += clearMarks(c.nxt);
  }
  return res;
}

void node::deleteChilds(HashingStrategy* h, char col, PosTransform curT, int lev, PnsStats& stats, bool onlyMarked){
  for(auto& child : childs){
    if(!onlyMarked || lev2 || parents.empty()){
      int found = 0;
      for(int i=0; i<(int) child.nxt->parents.size(); i++){
        if(child.nxt->parents[i] == this){
          child.nxt->parents[i] = child.nxt->parents.back();
          stats.generalStats->bytesUsedPNS -= child.nxt->parents.capacity() * sizeof(node*);
          child.nxt->parents.pop_back();
          stats.generalStats->bytesUsedPNS += child.nxt->parents.capacity() * sizeof(node*);
          found++;
          break;
        }
      }
      assert(found==1);
    }
    if(child.nxt->parents.empty() || onlyMarked){
      Pos absMove = curT.transform(child.move);
      if(lev < stats.maxHashLev)
        h->move(absMove, col);
      child.nxt->cleanUp(h, (char) (3-col), curT * child.transform, lev+1, stats, onlyMarked);
      if(lev < stats.maxHashLev)
        h->unmove(col);
      if(child.nxt->parents.empty()){
        stats.generalStats->bytesUsedPNS -= sizeof(node) + 36; //empirically determined approximation because memory overhead when allocating with new (might also be with vectors etc.)
        delete child.nxt;
      }
    }
  }
  if(!onlyMarked || lev2 || parents.empty()){
    stats.generalStats->bytesUsedPNS -= sizeof(childNode) * childs.capacity();
    childs.clear();
  }
  if(onlyMarked)
    lev2 = false;
}

void node::cleanUp(HashingStrategy* h, char col, PosTransform curT, int lev, PnsStats& stats, bool onlyMarked){
  if(parents.empty()){
    if(lev <= stats.maxHashLev)
      h->removeCurrentPos(this);
    stats.totDeleted++;
  }
  deleteChilds(h, col, curT, lev, stats, onlyMarked);
}

node::~node(){
  assert(childs.empty());
  assert(parents.empty());
}


PNS::PNS(Board* _b, GeneralStats* _generalStats, bool _firstLevel, long long _maxNodes, node* _root, int _startLev, HashingStrategy* _h)
    : b(_b), firstLevel(_firstLevel), startLev(_startLev) {
  if(stats.verbose && false)
    cout << DRAW_DEPTH << "    " << DRAW_WIN_COLOR << endl;

  stats.maxNodes = _maxNodes;
  stats.verbose = firstLevel;
  stats.drawDepth = DRAW_DEPTH;
  stats.maxHashLev = MAX_HASH_LEV + startLev;
  stats.generalStats = _generalStats;

  if(_root == NULL){
    Pos startPos(BOARD_SIZE/2, BOARD_SIZE/2);
    _root = new node(startPos, b, BLACK, 0, stats);
  }
  root = _root;
  stats.root = root;

  if(_h == NULL){
    newHashtable = true;
    _h = new HashingStrategy(b, _generalStats);
    for(int y=0; y<(int) b->board.size(); y++){
      for(int x=0; x<(int) b->board[0].size(); x++){
        if(b->board[y][x] != EMPTY){
          Pos p(y,x);
          _h->move(p, b->board[y][x]);
        }
      }
    }
  }
  h = _h;
}

PNS::~PNS(){
  if(firstLevel){
    root->cleanUp(h, 1, identityTransform(), 0, stats, false);
    delete(root);
  }
  if(newHashtable)
    delete(h);
}

void PNS::calcVals(node* cur, char col){
  (void) col; //get rid of unused warning -> TODO fix

  if(cur->mynum == 0 || cur->othnum == 0)
    return;
  cur->mynum = INF_PN+1;
  cur->othnum = 0;
#ifdef PN_STATS
  cur->myRealNum = 0;
  cur->othRealNum = 0;
#endif
#ifdef WEAK_PNS
  bool othInfty = false;
  int childsLeft = 0;
  long long maxNum = 0;
  for(auto& child : cur->childs){
    cur->mynum = min(cur->mynum, child.nxt->othnum);
    othInfty |= child.nxt->mynum > INF_PN;
    if(child.takeMax){
      maxNum = max(maxNum, child.nxt->mynum);
      if(child.nxt->mynum != 0)
        childsLeft++;
    }
    else
      cur->othnum = min(INF_PN, cur->othnum + child.nxt->mynum);
  }
  cur->othnum = min(INF_PN, cur->othnum + maxNum + max(0, childsLeft-1));
  if(othInfty)
    cur->othnum = INF_PN+1;
#else
  bool othInfty = false;
  for(auto child : cur->childs){
    cur->mynum = min(cur->mynum, child.nxt->othnum);
    cur->othnum = min(INF_PN, cur->othnum+child.nxt->mynum);
    othInfty |= child.nxt->mynum > INF_PN;
#ifdef PN_STATS
    if(child.nxt->othnum == 0)
      cur->myRealNum = child.nxt->othRealNum;
    cur->othRealNum = min(INF_PN, cur->othRealNum+child.nxt->myRealNum);
#endif
  }
  if(othInfty)
    cur->othnum = INF_PN+1;
#endif
}

void PNS::calcAllParentVals(node* cur, char col){
  queue<pair<node*,char>> Q;
  Q.push({cur, col});
  while(!Q.empty()){
    node* nxt;
    char nxtCol;
    tie(nxt, nxtCol) = Q.front();
    Q.pop();
    calcVals(nxt, nxtCol);
    nxt->vis = false;
    for(auto p : nxt->parents){
      if(!p->vis){
        p->vis = true;
        Q.push({p, 3-col});
      }
    }
  }
}

#ifdef WEAK_PNS
void markLCA(node* parent1, node* parent2, node* child1, node* child2){
  while(parent1 != parent2){
    child1 = parent1;
    child2 = parent2;
    parent1 = parent1->parents[0];
    parent2 = parent2->parents[0];
  }
  for(auto& child : parent1->childs){
    if(child.nxt == child1)
      child.takeMax = true;
  }
  for(auto& child : parent2->childs){
    if(child.nxt == child2)
      child.takeMax = true;
  }
}
#endif

long long nodesSecondLevel(long long totNodes){
  // empirically determined constants; a should be approx max nodes in memory
  const double a = 2e6;
  const double b = 2e5;
  double frac = 1.0/(1 + exp((a-(double)totNodes)/b));
  long long res = max(1ll, (long long)(frac*(double)totNodes));
  return res;
}

void PNS::dfs(node* cur, char col, int lev, PosTransform curT, long long mynumBound, long long othnumBound){
  stats.totNodesVisited++;
  // used for df-pns; this is only a single iteration otherwise (see break at the end)
  while(stats.totNodes < stats.maxNodes && cur->mynum < mynumBound && cur->othnum < othnumBound){
    // node is proven
    if(cur->mynum == 0 || cur->othnum == 0)
      return;
    if(cur->childs.empty()){ // leaf
      dfsLeaf(cur, col, lev, curT);
    }
    else{ // internal node
      dfsInternal(cur, col, lev, curT, mynumBound, othnumBound);
    }
    // only potentially deletes subtree of ancestor in dfs-tree, no other subtrees are deleted even though we update all parents' proof numbers
    if(cur->mynum == 0 || cur->othnum == 0){
#ifdef DELETE_CHILDS
      cur->deleteChilds(h, col, curT, lev, stats, false);
#endif
      stats.proved[lev]++;
    }
#ifndef DF_PNS
    break;
#endif
  }
}

inline void PNS::dfsLeaf(node* cur, char col, int lev, PosTransform curT){
  stats.leaves[lev]--;
  if(PNS2 && firstLevel){
    /*
      We start a new second level pns search, using the same hashtable, from the current node
      The nodes of the second level are limited by nodesSecondLevel(...)
      All nodes except direct childs are discarded after the second level search
     */
    HashingStrategy* secondH = lev < stats.maxHashLev ? h : NULL;
    PNS secondLevel(b, stats.generalStats, false, nodesSecondLevel(stats.totNodes), cur, lev, secondH);
    secondLevel.start(col, curT);
    for(auto c : cur->childs){
      auto newT = curT * c.transform;
      Pos absMove = curT.transform(c.move);
      secondLevel.h->move(absMove, col);
      c.nxt->deleteChilds(secondLevel.h, (char) (3-col), newT, lev+1, secondLevel.stats, true);
      secondLevel.h->unmove(col);
      c.nxt->lev2 = false;
    }
    cur->lev2 = false;
    stats.leaves[lev+1] += (int) cur->childs.size();
    stats.totNodes += secondLevel.stats.totNodes - secondLevel.stats.totDeleted;
  }
  else{
    /*
      No second level is used -> Just expand the current node and add all children
     */
    auto posMoves = b->getPosNextMoves();
    for(auto move : posMoves){
      b->move(move, col);
      if(lev < stats.maxHashLev)
        h->move(move, col);
      auto hashEntry = lev < stats.maxHashLev ? h->getCur(cur) : make_pair(identityTransform(), (node*) NULL);
      PosTransform transform = hashEntry.first;
      node* sameNode = hashEntry.second;
      PosTransform newT = transform;
#ifdef WEAK_PNS
      bool nodesMerged = sameNode != NULL;
#endif
      if(sameNode == NULL){ // no hash enty was found -> create new node
        stats.levCount[lev+1]++;
        sameNode = new node(move, b, (char) (3-col), lev+1, stats);
        if(lev < stats.maxHashLev)
          h->storeCurrentPos(sameNode, curT);
        newT = curT;
      }
      stats.generalStats->bytesUsedPNS -= sameNode->parents.capacity() * sizeof(node*);
      sameNode->parents.push_back(cur);
      sameNode->parents.shrink_to_fit(); //destroys amortization, but vector is small and resize does not happen often
      stats.generalStats->bytesUsedPNS += sameNode->parents.capacity() * sizeof(node*);

      b->unmove();
      if(lev < stats.maxHashLev)
        h->unmove(col);

      // calculate right transformation matrix along edge to child
      newT = curT.inv() * newT;
      auto childMove = curT.inv().transform(move);
      childNode child(childMove, sameNode, newT);
      cur->childs.push_back(child);
      cur->lev2 = true;
#ifdef WEAK_PNS
      if(nodesMerged){
        // weak pns: take max at lca of merged children
        markLCA(cur, sameNode->parents[0], sameNode, sameNode);
      }
#endif
    }
    cur->childs.shrink_to_fit(); // memory optimization, no performance problem as childs don't change
    stats.generalStats->bytesUsedPNS += cur->childs.capacity() * sizeof(childNode);
    stats.generalStats->intNodesAtLev[lev]++;
    stats.generalStats->intNodesChildrenAtLev[lev] += (int) cur->childs.size();
  }
  // update all proof numbers of ancestors
  calcAllParentVals(cur, col);

  stats.totExpanded++;
#ifdef DF_PNS
  // occasionally print stats - they are not printed in start-function for df-pns
  if(stats.verbose && (stats.totExpanded & ((1<<16)-1)) == 0)
    stats.print();
#endif
}

inline void PNS::dfsInternal(node* cur, char col, int lev, PosTransform curT, long long mynumBound, long long othnumBound){
#ifndef DF_PNS
  (void) mynumBound;
  (void) othnumBound;
#endif
  childNode *best = &(cur->childs[0]), *secondBest = NULL;
  for(auto& child : cur->childs){
    if(child.nxt->othnum < best->nxt->othnum){
      secondBest = best;
      best = &child;
    }
    else if(secondBest == NULL || child.nxt->othnum < secondBest->nxt->othnum){
      secondBest = &child;
    }
  }
  assert(best->nxt->othnum == cur->mynum);
  Pos absMove = curT.transform(best->move);
  b->move(absMove, col);
  if(lev < stats.maxHashLev)
    h->move(absMove, col);
#ifdef DF_PNS
  long long childNumBound = othnumBound - cur->othnum + best->nxt->mynum;
  long long childOthNumBound = min(mynumBound, secondBest->nxt->othnum + 1);
#else
  long long childNumBound = INF_PN+2;
  long long childOthNumBound = INF_PN+2;
#endif
  dfs(best->nxt, (char) (3-col), lev+1, curT * best->transform, childNumBound, childOthNumBound); //switch color
  b->unmove();
  if(lev < stats.maxHashLev)
    h->unmove(col);
}

void PNS::start(char toMove, PosTransform startTransform){
  if(firstLevel){
    stats.generalStats->startTime = stats.generalStats->currentTimeMillis();
    stats.generalStats->startClock = clock();
  }
  while(stats.totNodes < stats.maxNodes && root->mynum != 0 && root->othnum != 0){
    dfs(root, toMove, startLev, startTransform, INF_PN+2, INF_PN+2);
    // occasionally print stats
    if(stats.verbose && (stats.totExpanded & ((1<<16)-1)) == 0)
      stats.print();
  }
  if(firstLevel)
    stats.print();
  if(stats.verbose){
    if(root->mynum == 0)
      cout << "BLACK has won" << endl;
    else
      cout << "RED has won" << endl;
  }
}
#endif
