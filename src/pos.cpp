#ifndef POS_CPP
#define POS_CPP
#include "pns.h"
using namespace std;

void print(vector<Pos>& p){
  for(auto x : p){
    cout << "(" << (int)x.x << "," << (int)x.y << ") ";
  }
  cout << endl;
}

Pos::Pos(int _y, int _x) : y((coordType) _y), x((coordType) _x) {}

Pos Pos::nextPos(int dir){
  Pos newP(y, x);
  newP.moveTo(dir);
  return newP;
}

bool Pos::operator==(const Pos& other) const{
  return x == other.x && y == other.y;
}

bool Pos::operator<(const Pos& other) const{
  return x<other.x || (x == other.x && y < other.y);
}

#endif
