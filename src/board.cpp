#ifndef BOARD_CPP
#define BOARD_CPP
#include "pns.h"
using namespace std;

Board::Board(int length){
  vector<vector<vector<int>>*> finished = {&finishedCircles, &finishedLines, &finishedTriangles};
  for(auto& v : finished){
    v->resize(2);
    (*v)[0].resize(7); (*v)[1].resize(7);
    (*v)[0][0] = (*v)[1][0] = length*length;
  }
  circles = vector<vector<vector<int>>>(length, vector<vector<int>>(length, vector<int>(2)));
  triangles = vector<vector<vector<vector<int>>>>(length, vector<vector<vector<int>>>(length, vector<vector<int>>(2, vector<int>(2))));
  lines =     vector<vector<vector<vector<int>>>>(length, vector<vector<vector<int>>>(length, vector<vector<int>>(3, vector<int>(2))));

  board = vector<vector<char>>(length, vector<char>(length));
  inPosNextMoves = vector<vector<char>>(length, vector<char>(length));
}

bool Board::inBound(Pos& p){
  return p.x >= 0 && p.x < (int)board.size() && p.y >= 0 && p.y < (int)board.size();
}

void Board::move(Pos& p, char color){
  if(inBound(p)){
    assert(board[p.y][p.x] == EMPTY);
    board[p.y][p.x] = color;
    pastMoves.push(p);
    
    //keeping track of currently possible moves
    inPosNextMoves[p.y][p.x] = TO_DELETE;
    newFreeGenerated.push(vector<Pos>());
    newFreeGenerated.top().reserve(6);

    Pos adj = p.nextPos(LEFT_UP);
    for(int dir=0; dir<6; dir++){
      if(getPos(adj) == EMPTY && inPosNextMoves[adj.y][adj.x] != IN_CORRECT){
        if(inPosNextMoves[adj.y][adj.x] == OUT_CORRECT)
          posNextMoves.push_back(adj);
        inPosNextMoves[adj.y][adj.x] = IN_CORRECT;
        newFreeGenerated.top().push_back(adj);
      }
      adj.moveTo(dir);
    }
    updatePatternCnt(p, color, 1);
  }
  else{
    cerr << "Position " << (int)p.y << " " << (int)p.x << " not in bounds!" << endl;
    assert(false);
  }
}

Pos Board::unmove(){
  Pos p = pastMoves.top();
  pastMoves.pop();
  
  //These moves are no longer possible
  for(Pos& oldP : newFreeGenerated.top()){
    inPosNextMoves[oldP.y][oldP.x] = TO_DELETE;
  }
  newFreeGenerated.pop();
  if(inPosNextMoves[p.y][p.x] == OUT_CORRECT)
    posNextMoves.push_back(p);
  inPosNextMoves[p.y][p.x] = IN_CORRECT;
  updatePatternCnt(p, board[p.y][p.x], -1);
  board[p.y][p.x] = EMPTY;
  return p;
}

char Board::lastMoveCol(){
  return getPos(pastMoves.top());
}

int Board::numMoves(){
  return (int)pastMoves.size();
}

vector<Pos> Board::getPosNextMoves(){
  /*
    computes the possible next moves.
    Some of the moves might no longer be possible - they are lazily deleted by setting the TO_DELETE flag at the respective position.
    This yields amortized constant time deletion.
   */
  for(int i=0; i<(int)posNextMoves.size(); i++){
    if(inPosNextMoves[posNextMoves[i].y][posNextMoves[i].x] == TO_DELETE){
      inPosNextMoves[posNextMoves[i].y][posNextMoves[i].x] = OUT_CORRECT;
      swap(posNextMoves[i], posNextMoves.back());
      posNextMoves.pop_back();
      i--;
    }
  }
  return posNextMoves;
}

inline void Board::changeCnt(vector<int>& cnt, vector<vector<int>>& target, int col, int dif){
  if(cnt[1-col] == 0) target[col][cnt[col]] += dif;
}

void Board::updatePatternCntLine(Pos& p, char color, int dif){ //representor is to the left top
  for(int dir=3; dir<6; dir++){
    Pos cur = p;
    for(int i=0; i<LINE_LENGTH; i++){
      // optimized code - crucial for performance: changeCnt only relevent if other color has cnt=0
      bool ch = lines[cur.y][cur.x][dir-3][color] <= 1;
      if(ch)
        changeCnt(lines[cur.y][cur.x][dir-3], finishedLines, 1-color, -1);
      else
        changeCnt(lines[cur.y][cur.x][dir-3], finishedLines, color, -1);
      lines[cur.y][cur.x][dir-3][color] += dif;
      if(ch)
        changeCnt(lines[cur.y][cur.x][dir-3], finishedLines, 1-color, 1);
      else
        changeCnt(lines[cur.y][cur.x][dir-3], finishedLines, color, 1);
      cur.moveTo(dir);
    }
  }
}

void Board::updatePatternCntCircle(Pos& p, char color, int dif){ //representor on left top
  int dir=3;
  Pos cur = p;
  for(int i=0; i<6; i++){
    // optimized code - crucial for performance: changeCnt only relevent if other color has cnt=0
    bool ch = circles[cur.y][cur.x][color] <= 1;
    if(ch)
      changeCnt(circles[cur.y][cur.x], finishedCircles, 1-color, -1);
    else
      changeCnt(circles[cur.y][cur.x], finishedCircles, color, -1);
    circles[cur.y][cur.x][color] += dif;
    if(ch)
      changeCnt(circles[cur.y][cur.x], finishedCircles, 1-color, 1);
    else
      changeCnt(circles[cur.y][cur.x], finishedCircles, color, 1);
    dir = (dir+1)%6;
    cur.moveTo(dir);
  }
}

void Board::updatePatternCntTriangle(Pos& p, char color, int dif){
  for(int startDir=0; startDir<2; startDir++){
    int dir = startDir;
    Pos cur = p;
    for(int d=0; d<3; d++){
      for(int j=0; j<2; j++){
        // optimized code - crucial for performance: changeCnt only relevent if other color has cnt=0
        bool ch = triangles[cur.y][cur.x][startDir][color] <= 1;
        if(ch)
          changeCnt(triangles[cur.y][cur.x][startDir], finishedTriangles, 1-color, -1);
        else
          changeCnt(triangles[cur.y][cur.x][startDir], finishedTriangles, color, -1);
        triangles[cur.y][cur.x][startDir][color] += dif;
        if(ch)
          changeCnt(triangles[cur.y][cur.x][startDir], finishedTriangles, 1-color, 1);
        else
          changeCnt(triangles[cur.y][cur.x][startDir], finishedTriangles, color, 1);
        cur.moveTo(dir);
      }
      dir = (dir+2)%6;
    }
  }
}

void Board::updatePatternCnt(Pos& p, char color, int dif){
  if(color == 0){
    cerr << "error at " << (int)p.x << " " << (int)p.y << endl;
  }
  color--; //color elem {0,1}
  updatePatternCntLine(p, color, dif);
  updatePatternCntCircle(p, color, dif);
  updatePatternCntTriangle(p, color, dif);
}

char Board::getPos(Pos p){
  if(inBound(p)){
    return board[p.y][p.x];
  }
  else
    return -1;
}

void Board::print(){
  for(int i=0; i<(int)board.size(); i++){
    for(int j=0; j<i; j++)
      cout << " ";
    for(int x : board[i]){
      cout << x << " ";
    }
    cout << "\n";
  }
  cout << flush;
}

bool Board::hasWon(Pos st){
  char col = getPos(st);
  bool res = finishedCircles[col-1][6] || finishedLines[col-1][LINE_LENGTH] || finishedTriangles[col-1][6];
  return res;
}

#endif
