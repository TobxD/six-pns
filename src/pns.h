#ifndef PNS_H
#define PNS_H
#include <bits/stdc++.h>

#define DELETE_CHILDS
#define HEURISTIC_VALUES
//#define WEAK_PNS
//#define DF_PNS //This results in a minor speedup nodes/s, but PNs are inaccurate due to DAG-structure and WEAK_PNS (formula not adjusted to this case)
//#define PLAY //works only if undefined(DELETE_CHILDS)
//#define VERIFY //works only if undefined(DELETE_CHILDS)
//#define PN_STATS //only accurate if undefined(WEAK_PNS)
const bool PNS2 = false; //PNS^2 algorithm

// whether hashing is used at all
#define HASHING
// if hashing is used: which symmetries should be detected?
#define MIRROR
#define ROTATION

using namespace std;

// directions to move on the board
const char RIGHT=0, RIGHT_DOWN=1, LEFT_DOWN=2, LEFT=3, LEFT_UP=4, RIGHT_UP=5;
const char EMPTY=0, RED=1, BLACK=2;

// can be varied to 5 to allow 5-in-a-row as a winning shape
const int LINE_LENGTH = 6;

const long long INF_PN = 1e18;
const long long INF_LL = 1e18;
const int BOARD_SIZE = 90;

extern int DRAW_DEPTH; // set in main
extern int DRAW_WIN_COLOR; // set in main

typedef int coordType;
typedef long long powerHashType;

struct Pos{
  coordType y, x;
  Pos(int _y, int _x);

  Pos nextPos(int dir);
  // inlined here for better performance
  inline void moveTo(int dir){
    switch(dir){
      case RIGHT: x++; break;
      case RIGHT_DOWN: y++; break;
      case LEFT_DOWN: y++; x--; break;
      case LEFT: x--; break;
      case LEFT_UP: y--; break;
      case RIGHT_UP: y--; x++; break;
      default: cerr << "wrong dir" << endl;
               exit(1);
    }
  }
  bool operator==(const Pos& other) const;
  bool operator<(const Pos& other) const;
};

class HashingStrategy;
struct PnsStats;
struct GeneralStats;

void print(vector<Pos>& p);

// opposing to the description in the thesis, we have the y-coordinate flipped in the implementation such that (0,0) is the top-left corner.
class Board{
  private:
    // store past moves to undo them later
    stack<Pos> pastMoves;
    // new moves can generate new possible next moves -> these newly created one are stored to be undone later
    stack<vector<Pos>> newFreeGenerated;

    //over-approximation of possible next moves, might contain positions that are not possible
    vector<Pos> posNextMoves;
    // describes whether some move is not in posNextMoves, is in that vector, or is in there but should not be (is to be lazily deleted)
    const char OUT_CORRECT = 0, IN_CORRECT = 1, TO_DELETE = 2;
    vector<vector<char>> inPosNextMoves;

    // winning shapes: for each instance and color, the number of tokens already in it is stored at the representative (left top corner of the shape)
    vector<vector<vector<int>>> circles; //[y][x][color]
    vector<vector<vector<vector<int>>>> lines; //[y][x][dir][color]
    vector<vector<vector<vector<int>>>> triangles; //[y][x][dir][color]

    // four functions below are used to update the above data structures to keep track of potential winning patterns
    void changeCnt(vector<int>& cnt, vector<vector<int>>& target, int col, int dif);
    void updatePatternCntLine(Pos& p, char color, int dif);
    void updatePatternCntCircle(Pos& p, char color, int dif);
    void updatePatternCntTriangle(Pos& p, char color, int dif);

    // returns whether p is on the board
    bool inBound(Pos& p);

  public:
    vector<pair<Pos, char>> startMoves;
    // for each color: how many shapes have x tokens of the color and no token of the other color?
    vector<vector<int>> finishedCircles, finishedLines, finishedTriangles; //[color][numOccupiedInPattern] = #occurences
    //the board -> EMPTY, RED, or BLACK
    vector<vector<char>> board;

    Board(int length);

    void move(Pos& p, char color);
    Pos unmove();
    char lastMoveCol();
    int numMoves();
    vector<Pos> getPosNextMoves();

    /*
       updates the pattern couters:
       for each pattern instance and each color, the number of tokens of the color in this pattern instance are stored at the representative (left-top position) of the pattern instance
     */
    void updatePatternCnt(Pos& p, char color, int dif);
    char getPos(Pos p);
    void print();
    // returns whether the last token put on position st was a winning move
    bool hasWon(Pos st);
};

typedef array<int,3> vec;
typedef array<vec,3> matrix;

// init all matrices and inverses in PosTransform to cache for later
void initMatrices();
// how much memory is used for these matrices/inverses?
long long getTransformMatricesMemory();

class PosTransform{
  public:
    int M;
    PosTransform(int offsetX, int offsetY, bool mirror, char rot);
    PosTransform(int _M): M(_M) {}

    Pos transform(Pos p);
    Pos reverseTransform(Pos p);
    PosTransform inv();
    PosTransform operator*(const PosTransform b);

    bool operator<(const PosTransform & p) const{
      return M < p.M;
    }
};
PosTransform identityTransform();

struct childNode;

struct node{
  vector<childNode> childs;
  vector<node*> parents;

  // flags used e.g. for calcAllParentVals and clean-up of second level of PNS^2
  // We could compress them into a single char, but c++ struct is padded anyways so this does not affect the struc size
  bool vis = false;
  bool lev2 = false;

  long long mynum, othnum;
#ifdef PN_STATS
  // the number of actual leaves included in the proof, is significantly smaller than mynum/othnum -> int sufficient
  int myRealNum, othRealNum; //only defined if respective num equals zero
#endif
  node(Pos& move, Board* b, char col, int lev, PnsStats& stats);
  // calculate the proof numbers using proof numbers of children
  long long calcNum(Board* b, char col, bool toMove, int lev); //col: player to move at current position
  void deleteChilds(HashingStrategy* h, char col, PosTransform curT, int lev, PnsStats& stats, bool onlyMarked);
  // delete node from hashtable and all children recursively using deleteChilds
  void cleanUp(HashingStrategy* h, char col, PosTransform curT, int lev, PnsStats& stats, bool onlyMarked);
  ~node();
};

struct childNode{
  Pos move;
  node* nxt;
  // X: stored move -> board; visiting this node: X = X*transform
  PosTransform transform;
#ifdef WEAK_PNS
  // if takeMax: this child together with some other child has a common descendent -> use max for these childs to avoid overcounting
  bool takeMax = false;
#endif
  childNode(Pos _move, node* _nxt, PosTransform& t): move(_move), nxt(_nxt), transform(t){}
};

class PowerHash{
  private:
    // data structure too keep track of number of tokens places at certen x-/y-coordinate -> used to quickly compute minimum x- and y-coordinate
    int cntX[4*BOARD_SIZE], cntY[4*BOARD_SIZE];
    int xmin=4*BOARD_SIZE, ymin=4*BOARD_SIZE;
    
    GeneralStats* stats;

    // moves to be un-done later
    stack<Pos> moves;
    // current hash for both colors; to be updated incrementally with each move
    powerHashType curHash[2] = {0};
    // move m on the board is fromBoard.transform(m) in the hashing-space
    PosTransform toBoard, fromBoard;

  public:
    PowerHash(PosTransform _toBoard, GeneralStats* _stats);
    powerHashType hashBoard();
    //the moves p are board-moves
    void move(Pos& p, char col);
    void unmove(char col);
    // returns t: normalized with 0,0 left top corner -> board space
    PosTransform curPosTransform();
    // determines whether two nodes really correspond to symmetric positions, namely if the position at leaf is equivalent to curBoard. curParent is the parent node of a node (that might be created) with position curBoard
    bool isSame(Board* curBoard, node* leaf, node* curParent, PosTransform othToMe);
};

struct HashtableEntry{
  // highest bit used for flag whether entry used
  long long fullHash = -1;
  node* treeNode = NULL;
  // transformation to accomodate for symmetries -> needed for right transformation matrices along edges in case of a node-merger
  PosTransform transform = identityTransform();
  // time stamp the node was last put into the HT or accessed
  int lastUsedTime = 0;
};

class Hashtable{
  private:
    GeneralStats* stats;

    // the timestamp with only 32 bits might be two small for large runs -> increase counter only every timeSlowdownFactor'th time to mitigate the problem while still only using 32 bit
    const int timeSlowdownFactor = 100;
    // the current timeStamp
    int timeStamp = 1;
    // timeStampCnt2 \in [0; timeSlowdownFactor[; when too large: reset and increment timeStamp
    int timeStampCnt2 = 0;
    // the actual hashtable
    HashtableEntry *table;

  public:
    // number of bits used for HT
    static const int INDEX_BITS = 22;
    // number of entries with same index
    static const int ASSOCIATIVE_FACTOR = 1;
    static const long long INDEX_MASK = (1<<INDEX_BITS)-1;
    // hash table number of entries
    static const long long SIZE = (1ll<<INDEX_BITS)*ASSOCIATIVE_FACTOR;

    Hashtable(GeneralStats* _stats);
    ~Hashtable();
    HashtableEntry* getEntry(long long hash);
    void put(long long hash, PosTransform transform, node* treeNode);
    void remove(long long hash, node* treeNode);
};

/*

moves in subtree m; move on board b;
along edge matrix e: if previously b = T * m, then now T := T * e
powerhash:
  parameter: hash space to real board = toBoard; fromBoard = toBoard.inv()
  curPosTransform(): hashSpace -> boardspace (toBoard * offsetM(xmin-offset, ymin-offset))
hashing:
  stored: curPosTransform().inv() * curT: move space to hash space
  getCur: transform = curPosTransform() * hashEntry->transform: move space of hash entry -> current board space

 */

class HashingStrategy{
  private:
    Board* b;
    // transform: board moves of hashed node -> normalized (0,0 left top corner)
    Hashtable table;
    // Hashing algorithms for each symmetry
    vector<PowerHash*> hashAlgos;
    GeneralStats* stats;

    // generates lookup hash as xor of the hashes of all symmetries; returns index with smallest hash
    pair<powerHashType, int> getLookupHash();

  public:
    HashingStrategy(Board* _b, GeneralStats* _stats);
    ~HashingStrategy();
    void move(Pos& p, char col);
    void unmove(char col);
    void storeCurrentPos(node* x, PosTransform curT);
    void removeCurrentPos(node* x);
    // transform: from hashed board (returned node*) space -> current board space (curPostransform())
    // check whether there is a entry in the hashtable with the current board; for checking with sameNode, the parent is given; if no entry is found, the second value of the returned pair is NULL
    pair<PosTransform, node*> getCur(node* parent);
};

struct GeneralStats{
  // only approximations
  long long bytesUsedHT = 0;
  long long bytesUsedPNS = 0;
  long long bytesUsedTransform = 0;

  // number of entries deleted because a newer one was inserted
  long long hashTableDeletedTooFull = 0;
  // total number of elements currently stored in the hashtable
  long long hashTableEntries = 0;

  // millisecond timestamp of start time
  long long startTime;
  long long startClock;

  // interesting stats for isSame of powerHash
  long long isSameFound=0, isSameFailed=0;

  long long intNodesAtLev[100] = {};
  long long intNodesChildrenAtLev[100] = {};

  long long currentTimeMillis();
  void print(long long totNodes = -1);
};

struct PnsStats{
  //even if counted as first player win, odd otherwise; first player is the player that makes the first move after the given init sequence; this is always black.
  int drawDepth;
  int maxHashLev;

  GeneralStats* generalStats;

  // needed for proof number stats
  node* root = NULL;

  bool verbose = true;

  // limits maximal search memory, stops after using more than that
  long long maxNodes;

  // nodes that were expanded (children of these nodes were added)
  long long totExpanded = 0;
  // number of total nodes created in the tree (including later deleted ones)
  long long totNodes = 0;
  // nubmer of nodes deleted at some point
  long long totDeleted = 0;
  // number of nodes visited during the dfs
  long long totNodesVisited = 0;
  long long levCount[100] = {0};
  long long proved[100] = {0};
  long long leaves[100] = {0}; // only unproven ones

  void print();
};

class PNS{
  private:
    Board* b;
    // indicating whether the hashtable was newly created when creating the PNS instance and thus has to be deleted when destroying it
    bool newHashtable = false;
    HashingStrategy* h;
    PnsStats stats;
    // is this the first level of a two-level PNS^2?
    bool firstLevel;
    // usually 0, but can be higher if second level
    int startLev;

    // calculate proof- and disproof-numbers from children
    void calcVals(node* cur, char col);
    // update all ancestors' proof numbers
    void calcAllParentVals(node* cur, char col);
    // the core dfs of the PNS with two helper functions -> seaches for MPN and then expands it and updates ancestors
    void dfs(node* cur, char col, int lev, PosTransform curT, long long mynumBound, long long othnumBound);
    void dfsLeaf(node* cur, char col, int lev, PosTransform curT);
    void dfsInternal(node* cur, char col, int lev, PosTransform curT, long long mynumBound, long long othnumBound);

  public:
    node* root;

    PNS(Board* _b, GeneralStats* _generalStats, bool _firstLevel = true, long long _maxNodes = INF_LL, node* _root = NULL, int startLev=0, HashingStrategy* _h = NULL);
    ~PNS();
    void start(char toMove, PosTransform startTransform);
};

// check independently, whether the constructed proof-tree is consistent and proves the result; only enabled if defined(VERIFY)
void verifyCorrectness(node* root, Board& startBoard, char startCol);
#endif
