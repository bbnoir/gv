/****************************************************************************
  FileName     [ fddMgrV.cpp ]
  PackageName  [ fdd ]
  Synopsis     [ FDD Manager functions ]
  Author       [ ]
  Copyright    [ Copyright(c) 2023-present DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#include "fddMgrV.h"

#include <cassert>
#include <fstream>
#include <iomanip>

#include "fddNodeV.h"
#include "util.h"

using namespace std;

FddMgrV* fddMgrV = 0;

//----------------------------------------------------------------------
//    helper functions
//----------------------------------------------------------------------
FddNodeV getFddNodeV(const string& fddName) {
    int id;
    if (myStr2Int(fddName, id))
        return fddMgrV->getFddNodeV(id);
    else
        return fddMgrV->getFddNodeV(fddName);
}

//----------------------------------------------------------------------
//    class FddMgrV
//----------------------------------------------------------------------
void FddMgrV::init(size_t nin, size_t h, size_t c) {
    reset();
    _uniqueTable.init(h);
    _computedTable.init(c);

    FddNodeV::setFddMgrV(this);
    FddNodeVInt::_terminal = uniquify(0, 0, 0);
    FddNodeV::_one  = FddNodeV(FddNodeVInt::_terminal, FDD_POS_EDGEV);
    FddNodeV::_zero = FddNodeV(FddNodeVInt::_terminal, FDD_NEG_EDGEV);

    // For FDD with positive Davio: xi = 0 XOR 1*xi
    // left=0 (neg edge to terminal), right=1 (pos edge to terminal)
    // Complement edge rule: left must be positive => flip output and left
    // So xi = ~node(1, 1, i)
    _supports.reserve(nin + 1);
    _supports.push_back(FddNodeV::_one);
    for (size_t i = 1; i <= nin; ++i)
        _supports.push_back(
            FddNodeV(FddNodeV::_one(), FddNodeV::_one(), i, FDD_NEG_EDGEV));
}

void FddMgrV::restart() {
    size_t nin = _supports.size() - 1;
    size_t h   = _uniqueTable.numBuckets();
    size_t c   = _computedTable.size();
    init(nin, h, c);
}

void FddMgrV::reset() {
    _supports.clear();
    _fddArr.clear();
    _fddMap.clear();
    FddHash::iterator bi = _uniqueTable.begin();
    for (; bi != _uniqueTable.end(); ++bi)
        delete (*bi).second;
    _uniqueTable.reset();
    _computedTable.reset();
}

// Extract cofactors of f at level v in FDD sense.
// f = negCof XOR boolDiff * x_v
// If f doesn't depend on x_v, negCof = f, boolDiff = 0.
void FddMgrV::getCofactors(FddNodeV f, unsigned v, FddNodeV& negCof, FddNodeV& boolDiff) {
    if (f.getLevel() < v || f.isTerminal()) {
        negCof   = f;
        boolDiff = FddNodeV::_zero;
        return;
    }
    assert(f.getLevel() == v);
    // f = L XOR R*x_v  (if positive edge)
    // ~f = ~L XOR R*x_v (if negative edge)
    if (f.isNegEdge()) {
        negCof   = ~(f.getLeft());
        boolDiff = f.getRight();
    } else {
        negCof   = f.getLeft();
        boolDiff = f.getRight();
    }
}

// Create a canonical FDD node, applying:
// 1. Redundancy: if right == 0, return left (f doesn't depend on x_level)
// 2. Complement edge: if left is negative-edged, flip output and left
FddNodeV FddMgrV::makeNode(FddNodeV left, FddNodeV right, unsigned level) {
    if (right == FddNodeV::_zero) return left;

    bool outNeg = left.isNegEdge();
    FddNodeV canonLeft = outNeg ? ~left : left;

    FddNodeVInt* ni = uniquify(canonLeft(), right(), level);
    size_t ret = size_t(ni);
    if (outNeg) ret ^= FDD_NEG_EDGEV;
    return ret;
}

//----------------------------------------------------------------------
//    FDD XOR: f ^ g
//----------------------------------------------------------------------
FddNodeV
FddMgrV::fddXor(FddNodeV f, FddNodeV g) {
    // Terminal cases
    if (f == FddNodeV::_zero) return g;
    if (g == FddNodeV::_zero) return f;
    if (f == g) return FddNodeV::_zero;
    if (f == ~g) return FddNodeV::_one;

    // Symmetry: ensure f <= g for better cache hit rate
    if (f > g) {
        FddNodeV tmp = f; f = g; g = tmp;
    }

    // Cache lookup
    FddCacheKeyV k(f(), g(), FDD_OP_XOR);
    size_t ret_t;
    if (_computedTable.read(k, ret_t)) return ret_t;

    // Find top variable
    unsigned v = f.getLevel();
    if (g.getLevel() > v) v = g.getLevel();

    // Get cofactors at level v
    FddNodeV Lf, Rf, Lg, Rg;
    getCofactors(f, v, Lf, Rf);
    getCofactors(g, v, Lg, Rg);

    // Recurse: (Lf XOR Rf*x) XOR (Lg XOR Rg*x) = (Lf XOR Lg) XOR (Rf XOR Rg)*x
    FddNodeV newL = fddXor(Lf, Lg);
    FddNodeV newR = fddXor(Rf, Rg);

    FddNodeV result = makeNode(newL, newR, v);

    _computedTable.write(k, result());
    return result;
}

//----------------------------------------------------------------------
//    FDD AND: f & g
//----------------------------------------------------------------------
FddNodeV
FddMgrV::fddAnd(FddNodeV f, FddNodeV g) {
    // Terminal cases
    if (f == FddNodeV::_zero) return FddNodeV::_zero;
    if (g == FddNodeV::_zero) return FddNodeV::_zero;
    if (f == FddNodeV::_one) return g;
    if (g == FddNodeV::_one) return f;
    if (f == g) return f;
    if (f == ~g) return FddNodeV::_zero;

    // Symmetry
    if (f > g) {
        FddNodeV tmp = f; f = g; g = tmp;
    }

    // Cache lookup
    FddCacheKeyV k(f(), g(), FDD_OP_AND);
    size_t ret_t;
    if (_computedTable.read(k, ret_t)) return ret_t;

    // Find top variable
    unsigned v = f.getLevel();
    if (g.getLevel() > v) v = g.getLevel();

    // Get cofactors
    FddNodeV Lf, Rf, Lg, Rg;
    getCofactors(f, v, Lf, Rf);
    getCofactors(g, v, Lg, Rg);

    // f*g at x=0: Lf * Lg
    // f*g at x=1: (Lf XOR Rf) * (Lg XOR Rg)
    // Boolean diff of f*g = (f*g)|x=0 XOR (f*g)|x=1
    FddNodeV newL = fddAnd(Lf, Lg);
    FddNodeV posCofF = fddXor(Lf, Rf);
    FddNodeV posCofG = fddXor(Lg, Rg);
    FddNodeV posCofProd = fddAnd(posCofF, posCofG);
    FddNodeV newR = fddXor(newL, posCofProd);

    FddNodeV result = makeNode(newL, newR, v);

    _computedTable.write(k, result());
    return result;
}

FddNodeVInt*
FddMgrV::uniquify(size_t l, size_t r, unsigned i) {
    FddNodeVInt* n = 0;
    FddHashKeyV k(l, r, i);
    if (!_uniqueTable.check(k, n)) {
        n = new FddNodeVInt(l, r, i);
        _uniqueTable.forceInsert(k, n);
    }
    return n;
}

bool FddMgrV::addFddNodeV(unsigned id, size_t n) {
    if (id >= _fddArr.size()) {
        unsigned origSize = _fddArr.size();
        _fddArr.resize(id + 1);
        for (unsigned i = origSize; i < _fddArr.size(); ++i)
            _fddArr[i] = 0;
    } else if (_fddArr[id] != 0)
        return false;
    _fddArr[id] = n;
    return true;
}

FddNodeV
FddMgrV::getFddNodeV(unsigned id) const {
    if (id >= _fddArr.size()) return size_t(0);
    return _fddArr[id];
}

bool FddMgrV::addFddNodeV(const string& str, size_t n) {
    return _fddMap.insert(FddMapPair(str, n)).second;
}

void FddMgrV::forceAddFddNodeV(const string& str, size_t n) {
    _fddMap[str] = n;
}

FddNodeV
FddMgrV::getFddNodeV(const string& name) const {
    FddMapConstIter bi = _fddMap.find(name);
    if (bi == _fddMap.end()) return size_t(0);
    return (*bi).second;
}

//----------------------------------------------------------------------
//    Application: evaluate FDD with input pattern
//----------------------------------------------------------------------
// FDD evaluation: f = L XOR R*x
//   if x=0: f = L
//   if x=1: f = L XOR R
int FddMgrV::evalCube(const FddNodeV& node, const string& pattern) const {
    size_t v = node.getLevel();
    size_t n = pattern.size();
    if (n < v) {
        cout << "Error: " << pattern << " too short!!" << endl;
        return -1;
    }
    for (size_t i = 0; i < n; ++i) {
        if (pattern[i] != '0' && pattern[i] != '1') {
            cout << "Illegal pattern: " << pattern[i] << "(" << i << ")" << endl;
            return -1;
        }
    }
    return evalCubeRecur(node, pattern);
}

int FddMgrV::evalCubeRecur(const FddNodeV& node, const string& pattern) const {
    if (node.isTerminal()) return node.isPosEdge() ? 1 : 0;

    unsigned level = node.getLevel();
    bool neg = node.isNegEdge();

    FddNodeV left  = node.getLeft();
    FddNodeV right = node.getRight();

    int leftVal = evalCubeRecur(left, pattern);

    char xi = pattern[level - 1];
    int result;
    if (xi == '0') {
        result = leftVal;
    } else {
        int rightVal = evalCubeRecur(right, pattern);
        result = leftVal ^ rightVal;
    }

    if (neg) result = 1 - result;
    return result;
}

bool FddMgrV::drawFdd(const string& name, const string& fileName) const {
    FddNodeV node = ::getFddNodeV(name);
    if (node() == 0) {
        cout << "Error: \"" << name << "\" is not a legal FDD node!!" << endl;
        return false;
    }
    ofstream ofile(fileName.c_str());
    if (!ofile) {
        cout << "Error: cannot open file \"" << fileName << "\"!!" << endl;
        return false;
    }
    node.drawFdd(name, ofile);
    return true;
}
