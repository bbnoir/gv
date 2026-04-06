/****************************************************************************
  FileName     [ fddMgrV.h ]
  PackageName  [ fdd ]
  Synopsis     [ Define FDD Manager ]
  Author       [ ]
  Copyright    [ Copyright(c) 2023-present DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#pragma once

#include <map>

#include "fddNodeV.h"
#include "myHashMap.h"

using namespace std;

class FddNodeV;

typedef vector<size_t> FddArr;
typedef map<string, size_t> FddMap;
typedef pair<string, size_t> FddMapPair;
typedef map<string, size_t>::const_iterator FddMapConstIter;

extern FddMgrV* fddMgrV;

class FddHashKeyV {
public:
    FddHashKeyV(size_t l, size_t r, unsigned i) : _l(l), _r(r), _i(i) {}

    size_t operator()() const { return ((_l << 3) + (_r << 3) + _i); }

    bool operator==(const FddHashKeyV& k) {
        return (_l == k._l) && (_r == k._r) && (_i == k._i);
    }

private:
    size_t _l;
    size_t _r;
    unsigned _i;
};

class FddCacheKeyV {
public:
    FddCacheKeyV() {}
    FddCacheKeyV(size_t f, size_t g, size_t op) : _f(f), _g(g), _op(op) {}

    size_t operator()() const { return ((_f << 3) + (_g << 3) + (_op << 3)); }

    bool operator==(const FddCacheKeyV& k) const {
        return (_f == k._f) && (_g == k._g) && (_op == k._op);
    }

private:
    size_t _f;
    size_t _g;
    size_t _op;
};

// Operation tags for the computed table
enum FddOpType {
    FDD_OP_XOR = 0,
    FDD_OP_AND = 1,
};

class FddMgrV {
    typedef HashMap<FddHashKeyV, FddNodeVInt*> FddHash;
    typedef Cache<FddCacheKeyV, size_t> FddCache;

public:
    FddMgrV(size_t nin = 128, size_t h = 8009, size_t c = 30011) { init(nin, h, c); }
    ~FddMgrV() { reset(); }

    void init(size_t nin, size_t h, size_t c);
    void restart();

    // Core FDD operations
    FddNodeV fddXor(FddNodeV f, FddNodeV g);
    FddNodeV fddAnd(FddNodeV f, FddNodeV g);

    const FddNodeV& getSupport(size_t i) const { return _supports[i]; }
    size_t getNumSupports() const { return _supports.size(); }

    FddNodeVInt* uniquify(size_t l, size_t r, unsigned i);

    bool addFddNodeV(unsigned id, size_t nodeV);
    FddNodeV getFddNodeV(unsigned id) const;

    bool addFddNodeV(const string& nodeName, size_t nodeV);
    void forceAddFddNodeV(const string& nodeName, size_t nodeV);
    FddNodeV getFddNodeV(const string& nodeName) const;

    int evalCube(const FddNodeV& node, const string& vector) const;
    bool drawFdd(const string& nodeName, const string& dotFile) const;

private:
    // level 0 = terminal; levels 1..nin = support variables
    vector<FddNodeV> _supports;
    FddHash _uniqueTable;
    FddCache _computedTable;

    FddArr _fddArr;
    FddMap _fddMap;

    void reset();

    // Helper: get FDD cofactors of f at level v
    void getCofactors(FddNodeV f, unsigned v, FddNodeV& negCof, FddNodeV& boolDiff);

    // Helper: create a canonical FDD node (apply complement edge rule + redundancy)
    FddNodeV makeNode(FddNodeV left, FddNodeV right, unsigned level);

    int evalCubeRecur(const FddNodeV& node, const string& pattern) const;
};
