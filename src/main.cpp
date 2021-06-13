#ifndef MAIN_CPP
#define MAIN_CPP
#include "pns.h"
using namespace std;

extern int DRAW_DEPTH;
extern int DRAW_WIN_COLOR;

#if !defined(DELETE_CHILDS) && defined(PLAY)
void play(Board& b, node* cur, PosTransform curT){
  string command;
  while(true){
    b.print();
    cout << "your move (absolute coordinates): " << flush;
    cin >> command;
    if(command == "b")
      break;
    cout << "read: " << command << endl;
    int x, y;
    cin >> x >> y;
    cout << "read: " << x << " " << y << endl;
    Pos p(y, x);
    node* nxt1 = NULL;
    for(auto [move,child,t] : cur->childs){
      if(curT.transform(move) == p){
        nxt1 = child;
        break;
      }
    }
    if(nxt1 == NULL){
      cerr << "illegal move" << endl;
    }
    else{
      b.move(p, BLACK);
      childNode* nxt2 = NULL;
      for(auto& child : nxt1->childs){
        if(child.nxt->othnum == nxt1->mynum){
          nxt2 = &child;
          break;
        }
      }
      auto absMove = curT.transform(nxt2->move);
      b.move(absMove, RED);
      cout << "moving " << (int)absMove.x << " " << (int)absMove.y << endl;
      play(b, nxt2->nxt, curT * nxt2->transform);
      b.unmove();
      b.unmove();
    }
  }
}
#endif

int main(int argc, char** input){
  Board b(BOARD_SIZE);
  if(argc != 2){
    cerr << "usage: " << input[0] << " <inputfile>" << endl;
    return 1;
  }

  initMatrices();

  const int offset = BOARD_SIZE/2;

  bool readFromCin = string(input[1]) == "-";
  ifstream reader(input[1]);

  int drawDepth;
  if(readFromCin)
    cin >> drawDepth;
  else
    reader >> drawDepth;

  DRAW_DEPTH = drawDepth;
  DRAW_WIN_COLOR = 1-(DRAW_DEPTH&1) + 1;

  string col;
  while(true){
    if(readFromCin && !(cin >> col))
      break;
    if(!readFromCin && !(reader >> col))
      break;
    char colNum = -1;
    if(col == "r")
      colNum = RED;
    else if(col == "b")
      colNum = BLACK;
    else
      cerr << "wrong color " << col << endl, exit(1);
    int x, y;
    if(readFromCin)
      cin >> x >> y;
    else
      reader >> x >> y;
    x += offset;
    y += offset;
    Pos p(y, x);
    b.move(p, colNum);
    b.startMoves.emplace_back(p, colNum);
  }
  reader.close();
  b.print();

  GeneralStats* generalStats = new GeneralStats();
  generalStats->bytesUsedHT = sizeof(HashtableEntry) * Hashtable::SIZE;
  generalStats->bytesUsedTransform += getTransformMatricesMemory();

  PNS pns(&b, generalStats);
  pns.start(BLACK, identityTransform());

#if !defined(DELETE_CHILDS) && defined(PLAY)
  play(b, pns.root, identityTransform());
#endif
#if !defined(DELETE_CHILDS) && defined(VERIFY)
  verifyCorrectness(pns.root, b, BLACK);
#endif

  return 0;
}
#endif
