#ifndef CHECKER_CPP
#define CHECKER_CPP
#include "pns.h"
using namespace std;

long long cnt = 0;

bool hasLine(Board& b, Pos st, int dir, char col){
  bool res = true;
  Pos cur = st;
  for(int i=0; res && i<LINE_LENGTH; i++){
    res &= b.getPos(cur) == col;
    cur.moveTo(dir);
  }
  return res;
}

bool hasTriangle(Board& b, Pos st, int dir, char col){
  bool works = true;
  for(int d=0; d<3; d++){
    for(int j=0; j<2; j++){
      st.moveTo(dir);
      if(b.getPos(st) != col){
        works = false;
        break;
      }
    }
    dir = (dir+2)%6;
  }
  return works;
}

bool hasCircle(Board& b, Pos st, int dir, char col){ //st must have col
  bool res = true;
  for(int d=0; res && d<5; d++){
    st.moveTo(dir);
    res &= b.getPos(st) == col;
    dir = (dir+1)%6;
  }
  return res;
}

bool hasWon(Board& b, Pos st){
  char col = b.getPos(st);
  bool res = false;
  for(int dir=0; !res && dir<3; dir++){
    res |= hasLine(b, st, dir, col);
  }
  res = res || hasTriangle(b, st, 0, col);
  res = res || hasTriangle(b, st, 1, col);
  res = res || hasCircle(b, st, 0, col);
  return res;
}

// return: 0: no one has won; 1/2: respective color has won
char determineWinner(Board& b){
  for(int y=6; y<BOARD_SIZE-6; y++){
    for(int x=6; x<BOARD_SIZE-6; x++){
      Pos p(y, x);
      if(b.getPos(p) != EMPTY && hasWon(b, p))
        return b.getPos(p);
    }
  }

  return 0;
}

set<Pos> possibleMoves(Board& b){
  set<Pos> res;
  for(int i=0; i<(int) b.board.size(); i++){
    for(int j=0; j<(int) b.board[0].size(); j++){
      Pos cur(i,j);
      if(b.getPos(cur) != 0)
        continue;
      for(int dir=0; dir<6; dir++){
        if(b.getPos(cur.nextPos(dir)) >= 1){
          res.insert(cur);
          break;
        }
      }
    }
  }
  return res;
}

set<Pos> moveSet(node* cur, PosTransform t){
  set<Pos> res;
  for(auto& child : cur->childs){
    res.insert(t.transform(child.move));
  }
  return res;
}

bool assureWin(Board& b, char col, int maxDepth);

bool assureLoss(Board& b, char col, int maxDepth = 2){
  if(maxDepth == 0)
    return false;
  char winner = determineWinner(b);
  if(winner != 0)
    return winner != col;
  auto moves = possibleMoves(b);
  bool looses = true;
  for(auto move : moves){
    b.move(move, col);
    looses &= assureWin(b, (char)(3-col), maxDepth-1);
    b.unmove();
  }
  return looses;
}

bool assureWin(Board& b, char col, int maxDepth = 2){
  if(maxDepth == 0)
    return false;
  char winner = determineWinner(b);
  if(winner != 0)
    return winner == col;
  auto moves = possibleMoves(b);
  for(auto move : moves){
    b.move(move, col);
    bool wins = assureLoss(b, (char)(3-col), maxDepth-1);
    b.unmove();
    if(wins)
      return true;
  }
  return false;
}

/*
verifies subtree:
- if no childs then right player has won
- the childs are exactly the set of possible moves
- the childs relevant for this subtree are correct
- given correct childs, the winner is correctly set
 */
void verify(node* cur, Board& b, PosTransform curTransform, char col){
  cnt++;
  assert(cur->mynum == 0 || cur->othnum == 0);
  char winner = determineWinner(b);
  if(cur->childs.empty()){
    assert((cur->mynum == 0 && assureWin(b, col)) || (cur->othnum == 0 && assureLoss(b, col)));
    return;
  }
  else{
    assert(winner == 0);
  }
  assert(possibleMoves(b) == moveSet(cur, curTransform));

  for(auto& child : cur->childs){
    auto realMove = curTransform.transform(child.move);
    assert(6 <= min(realMove.y, realMove.x) && BOARD_SIZE-6 > max(realMove.y, realMove.x));
    assert(b.getPos(realMove) == EMPTY);
    assert(cur->othnum != 0 || child.nxt->mynum == 0);
    if(cur->othnum == 0 || (cur->mynum == 0 && child.nxt->othnum == 0)){
      b.move(realMove, col);
      verify(child.nxt, b, curTransform * child.transform, (char) (3-col));
      b.unmove();
      if(cur->mynum == 0)
        return;
    }
  }
  assert(cur->mynum != 0);
}

void verifyCorrectness(node* root, Board& startBoard, char startCol){
  cout << "starting to verify correctness" << endl;
  verify(root, startBoard, identityTransform(), startCol);
  cout << "correctness of " << cnt << " nodes verified" << endl;
}

#endif
