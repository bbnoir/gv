/****************************************************************************
  FileName     [ fddNodeV.h ]
  PackageName  [ fdd ]
  Synopsis     [ Define basic FDD Node data structures ]
  Author       [ ]
  Copyright    [ Copyright(c) 2023-present DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#pragma once

#include <iostream>
#include <map>
#include <vector>

using namespace std;

#define FDD_EDGEV_BITS 2
#define FDD_NODE_PTR_MASKV ((~(size_t(0)) >> FDD_EDGEV_BITS) << FDD_EDGEV_BITS)

class FddMgrV;
class FddNodeVInt;

enum FDD_EDGEV_FLAG {
    FDD_POS_EDGEV = 0,
    FDD_NEG_EDGEV = 1,
    FDD_EDGEV_DUMMY
};

class FddNodeV {
public:
    static FddNodeV _one;
    static FddNodeV _zero;
    static bool _debugFddAddr;
    static bool _debugRefCount;

    FddNodeV() : _nodeV(0) {}
    FddNodeV(size_t l, size_t r, size_t i, FDD_EDGEV_FLAG f = FDD_POS_EDGEV);
    FddNodeV(const FddNodeV& n);
    FddNodeV(FddNodeVInt* n, FDD_EDGEV_FLAG f = FDD_POS_EDGEV);
    FddNodeV(size_t v);
    ~FddNodeV();

    const FddNodeV& getLeft() const;
    const FddNodeV& getRight() const;
    unsigned getLevel() const;
    unsigned getRefCount() const;
    bool isNegEdge() const { return (_nodeV & FDD_NEG_EDGEV); }
    bool isPosEdge() const { return !isNegEdge(); }

    // FDD cofactors: left = fx_bar (neg cofactor), right = fd (bool diff)
    // For complement edge ~f = ~left XOR right*x, so only left is flipped.
    FddNodeV getNegCof(unsigned i) const;
    FddNodeV getBoolDiff(unsigned i) const;

    size_t operator()() const { return _nodeV; }
    FddNodeV operator~() const { return (_nodeV ^ FDD_NEG_EDGEV); }
    FddNodeV& operator=(const FddNodeV& n);
    FddNodeV operator&(const FddNodeV& n) const;
    FddNodeV& operator&=(const FddNodeV& n);
    FddNodeV operator|(const FddNodeV& n) const;
    FddNodeV& operator|=(const FddNodeV& n);
    FddNodeV operator^(const FddNodeV& n) const;
    FddNodeV& operator^=(const FddNodeV& n);
    bool operator==(const FddNodeV& n) const { return (_nodeV == n._nodeV); }
    bool operator!=(const FddNodeV& n) const { return (_nodeV != n._nodeV); }
    bool operator<(const FddNodeV& n) const;
    bool operator<=(const FddNodeV& n) const;
    bool operator>(const FddNodeV& n) const { return !((*this) <= n); }
    bool operator>=(const FddNodeV& n) const { return !((*this) < n); }

    friend ostream& operator<<(ostream& os, const FddNodeV& n);

    void drawFdd(const string&, ofstream&) const;
    string getLabel() const;

    static void setFddMgrV(FddMgrV* m) { _FddMgrV = m; }

    friend class FddMgrV;

private:
    size_t _nodeV;
    static FddMgrV* _FddMgrV;

    FddNodeVInt* getFddNodeVInt() const {
        return (FddNodeVInt*)(_nodeV & FDD_NODE_PTR_MASKV);
    }
    bool isTerminal() const;
    void print(ostream&, size_t, size_t&) const;
    void unsetVisitedRecur() const;
    void drawFddRecur(ofstream&) const;
};

class FddNodeVInt {
    friend class FddNodeV;
    friend class FddMgrV;

    FddNodeVInt() : _level(0), _refCount(0), _visited(0) {}
    FddNodeVInt(size_t l, size_t r, unsigned ll)
        : _left(l), _right(r), _level(ll), _refCount(0), _visited(0) {}

    const FddNodeV& getLeft() const { return _left; }
    const FddNodeV& getRight() const { return _right; }
    unsigned getLevel() const { return _level; }
    unsigned getRefCount() const { return _refCount; }
    void incRefCount() { ++_refCount; }
    void decRefCount() { --_refCount; }
    bool isVisited() const { return (_visited == 1); }
    void setVisited() { _visited = 1; }
    void unsetVisited() { _visited = 0; }

    FddNodeV _left;
    FddNodeV _right;
    unsigned _level : 16;
    unsigned _refCount : 15;
    unsigned _visited : 1;

    static FddNodeVInt* _terminal;
};
