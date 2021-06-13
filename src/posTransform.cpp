#ifndef POS_TRANSFORM_CPP
#define POS_TRANSFORM_CPP

#include "pns.h"

using namespace std;

// opposing to the description in the thesis, we have the y-coordinate flipped in the implementation such that (0,0) is the top-left corner.

matrix identity {{ {1,0,0},{0,1,0},{0,0,1} }};
// mirror and rotation include shifts to rotate/mirror around center
matrix mirrorM {{ {1, 1, -BOARD_SIZE/2},{0, -1, BOARD_SIZE},{0, 0, 1} }};
matrix oneRot {{ {1, 1, -BOARD_SIZE/2},{-1, 0, BOARD_SIZE},{0, 0, 1} }};
matrix rotM[6];

const int dim = 16*BOARD_SIZE+4;
const int offset = dim/2;
const int maxCoord = offset/4;
vector<matrix> matrices(dim*dim*2*6); //[x+offset][y+offset][mirror][rot]
int invM[dim*dim*2*6];
int mapBack[3][3][3][3][dim][dim];

long long getTransformMatricesMemory(){
  // lots of memory is used, but this is only constant and reduces other memory and computation (-> inverses)
  long long res = 0;
  res += sizeof(matrix) * matrices.capacity(); //matrices
  res += sizeof(int) * dim*dim*2*6; //invM
  res += sizeof(int) * 3*3*3*3*dim*dim; //mapBack
  return res;
}

void print(matrix M){
  cout << "[";
  for(int i=0; i<3; i++){
    cout << "[";
    for(int j=0; j<3; j++){
      cout << M[i][j] << ", ";
    }
    cout << "]" << endl;
  }
  cout << "]" << endl;
}

void print(PosTransform p){
  print(matrices[p.M]);
}

matrix offsetM(int x, int y){
  return {{ {1, 0, x},{0, 1, y},{0, 0, 1} }};
}

matrix invertMatrix(matrix m){
  int det = m[0][0]*m[1][1] - m[1][0]*m[0][1]; // actually should be 1/x but x is fine since abs(x) == 1
  matrix res {{ {m[1][1]*det, -m[0][1]*det, -det*(m[0][2]*m[1][1]-m[0][1]*m[1][2])},
            {-m[1][0]*det, m[0][0]*det, det*(m[0][2]*m[1][0]-m[0][0]*m[1][2])},
            {0, 0, 1} }};
  return res;
}

matrix mult(matrix& a, matrix& b){
  matrix res {};
  // only two rows computed , third is {0,0,1}
  for(int i=0; i<2; i++){
    for(int j=0; j<3; j++){
      for(int k=0; k<3; k++){
        res[i][j] += a[i][k] * b[k][j];
      }
    }
  }
  res[2][2] = 1; // assumes last row of matrix is {0, 0, 1}
  return res;
}

matrix computeM(int offsetX, int offsetY, bool mirror, int rot){
  matrix curM = identity;
  if(mirror)
    curM = mult(mirrorM, curM);
  if(offsetX || offsetY){
    auto offM = offsetM(offsetX, offsetY);
    curM = mult(offM, curM);
  }
  if(rot){
    auto rM = rotM[rot];
    curM = mult(rM, curM);
  }
  return curM;
}

// get index for matrices array
int getInd(int offsetX, int offsetY, bool mirror, int rot){
  return ((((offsetX+offset)*dim) + offsetY+offset)*2 + mirror)*6 + rot;
}

int* matrixEntry(matrix& M){
  int a = M[0][0]+1;
  int b = M[0][1]+1;
  int c = M[1][0]+1;
  int d = M[1][1]+1;
  int e = M[0][2]+offset;
  int f = M[1][2]+offset;
  return &mapBack[a][b][c][d][e][f];
}

void initMatrices(){
  rotM[0] = identity;
  for(int i=1; i<6; i++){
    rotM[i] = mult(oneRot, rotM[i-1]);
  }

  // compute all matrices in the matrices array
  for(int i=-maxCoord; i<maxCoord; i++){
    for(int j=-maxCoord; j<maxCoord; j++){
      for(int k=0; k<2; k++){
        for(int l=0; l<6; l++){
          int ind = getInd(i, j, k, l);
          matrices[ind] = computeM(i, j, k, l);
          *matrixEntry(matrices[ind]) = ind;
        }
      }
    }
  }

  // calculate inverse matrices
  for(int i=-maxCoord; i<maxCoord; i++){
    for(int j=-maxCoord; j<maxCoord; j++){
      for(int k=0; k<2; k++){
        for(int l=0; l<6; l++){
          int ind = getInd(i, j, k, l);
          auto inv = invertMatrix(matrices[ind]);
          invM[ind] = *matrixEntry(inv); // Can be 0 -> checked at other point
        }
      }
    }
  }
}

/*
  Matrix to index in matrices-array
 */
int mapToIndex(matrix M){
  int res = *matrixEntry(M);
  return res;
}

int mult(int m1, int m2){
  auto matr = mult(matrices[m1], matrices[m2]);
  int ind = mapToIndex(matr);
  if(ind == 0){
    // matrix was not precomputed -> compute it now
    ind = (int) matrices.size();
    auto entry = matrixEntry(matr);
    *entry = ind;
    matrices.push_back(matr);
    invM[ind] = mapToIndex(invertMatrix(matr));
  }
  return ind;
}

PosTransform identityTransform(){
  PosTransform t(0, 0, false, 0);
  return t;
}

vec vectorFromPos(Pos p){
  return {p.x, p.y, 1};
}
Pos posFromVector(vec v){
  return Pos(v[1], v[0]);
}

PosTransform PosTransform::operator*(const PosTransform b){
  assert(M != 0);
  assert(b.M != 0);
  PosTransform res(0,0,false,0);
  res.M = mult(M, b.M);
  if(res.M == 0){
    print(matrices[M]);
    print(matrices[b.M]);
    auto R = mult(matrices[M], matrices[b.M]);
    print(R);
    cerr << "produces matrix that is not precomputed" << endl;
    int x = b.M + res.M;
    assert(x != -1);
    assert(false);
  }
  return res;
}

PosTransform PosTransform::inv(){
  assert(M != 0);
  PosTransform res(invM[M]);
  assert(res.M != 0);
  return res;
}

PosTransform::PosTransform(int offsetX, int offsetY, bool mirror, char rot){
  M = getInd(offsetX, offsetY, mirror, rot);
  assert(M != 0);
}
Pos PosTransform::transform(Pos p){
  auto& m = matrices[M];
  int x = p.x*m[0][0] + p.y*m[0][1] + m[0][2];
  int y = p.x*m[1][0] + p.y*m[1][1] + m[1][2];
  auto res = Pos(y, x);
  return res;
}
Pos PosTransform::reverseTransform(Pos p){
  return inv().transform(p);
}

#endif
