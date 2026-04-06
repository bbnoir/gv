/****************************************************************************
  FileName     [ fddNodeV.cpp ]
  PackageName  [ fdd ]
  Synopsis     [ Define FDD Node member functions ]
  Author       [ ]
  Copyright    [ Copyright(c) 2023-present DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#include "fddNodeV.h"

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <sstream>

#include "fddMgrV.h"

FddMgrV* FddNodeV::_FddMgrV         = 0;
FddNodeVInt* FddNodeVInt::_terminal = 0;
FddNodeV FddNodeV::_one;
FddNodeV FddNodeV::_zero;
bool FddNodeV::_debugFddAddr  = false;
bool FddNodeV::_debugRefCount = false;

FddNodeV::FddNodeV(size_t l, size_t r, size_t i, FDD_EDGEV_FLAG f) {
    FddNodeVInt* n = _FddMgrV->uniquify(l, r, i);
    assert(n != 0);
    _nodeV = size_t(n) + f;
    n->incRefCount();
}

FddNodeV::FddNodeV(const FddNodeV& n) : _nodeV(n._nodeV) {
    FddNodeVInt* t = getFddNodeVInt();
    if (t) t->incRefCount();
}

FddNodeV::FddNodeV(FddNodeVInt* n, FDD_EDGEV_FLAG f) {
    assert(n != 0);
    _nodeV = size_t(n) + f;
    n->incRefCount();
}

FddNodeV::FddNodeV(size_t v) : _nodeV(v) {
    FddNodeVInt* n = getFddNodeVInt();
    if (n) n->incRefCount();
}

FddNodeV::~FddNodeV() {
    FddNodeVInt* n = getFddNodeVInt();
    if (n) n->decRefCount();
}

const FddNodeV&
FddNodeV::getLeft() const {
    assert(getFddNodeVInt() != 0);
    return getFddNodeVInt()->getLeft();
}

const FddNodeV&
FddNodeV::getRight() const {
    assert(getFddNodeVInt() != 0);
    return getFddNodeVInt()->getRight();
}

unsigned
FddNodeV::getLevel() const {
    return getFddNodeVInt()->getLevel();
}

unsigned
FddNodeV::getRefCount() const {
    return getFddNodeVInt()->getRefCount();
}

// FDD negative cofactor at level i.
// For f = L XOR R*xi:  f|xi=0 = L
// For ~f: ~f|xi=0 = ~L  (complement only affects left child)
FddNodeV
FddNodeV::getNegCof(unsigned i) const {
    assert(getFddNodeVInt() != 0);
    assert(i > 0);
    if (i > getLevel()) return (*this);
    if (i == getLevel()) {
        return isNegEdge() ? ~getLeft() : getLeft();
    }
    // i < getLevel(): recurse
    FddNodeV l = getLeft().getNegCof(i);
    FddNodeV r = getRight().getNegCof(i);
    // Rebuild node at this level with new cofactors
    // Need to handle complement edge: if output is neg, flip left child
    if (r == FddNodeV::_zero) {
        return isNegEdge() ? ~l : l;
    }
    bool outNeg = isNegEdge() ^ l.isNegEdge();
    FddNodeV newL = l.isNegEdge() ? ~l : l;
    FddNodeV newR = r;
    FddNodeVInt* ni = _FddMgrV->uniquify(newL(), newR(), getLevel());
    size_t ret = size_t(ni);
    if (outNeg) ret ^= FDD_NEG_EDGEV;
    return ret;
}

// FDD Boolean difference at level i.
// For f = L XOR R*xi:  fd_xi = R
// Complement edge does NOT affect right child: ~f = ~L XOR R*xi => fd still R
FddNodeV
FddNodeV::getBoolDiff(unsigned i) const {
    assert(getFddNodeVInt() != 0);
    assert(i > 0);
    if (i > getLevel()) return FddNodeV::_zero;
    if (i == getLevel()) {
        return getRight();
    }
    // i < getLevel(): recurse
    FddNodeV l = getLeft().getBoolDiff(i);
    FddNodeV r = getRight().getBoolDiff(i);
    if (r == FddNodeV::_zero) {
        // Result doesn't depend on this level's variable either
        return isNegEdge() ? ~l : l;
    }
    bool outNeg = isNegEdge() ^ l.isNegEdge();
    FddNodeV newL = l.isNegEdge() ? ~l : l;
    FddNodeV newR = r;
    FddNodeVInt* ni = _FddMgrV->uniquify(newL(), newR(), getLevel());
    size_t ret = size_t(ni);
    if (outNeg) ret ^= FDD_NEG_EDGEV;
    return ret;
}

FddNodeV&
FddNodeV::operator=(const FddNodeV& n) {
    FddNodeVInt* t = getFddNodeVInt();
    if (t) t->decRefCount();
    _nodeV = n._nodeV;
    t = getFddNodeVInt();
    if (t) t->incRefCount();
    return (*this);
}

FddNodeV
FddNodeV::operator^(const FddNodeV& n) const {
    return _FddMgrV->fddXor((*this), n);
}

FddNodeV&
FddNodeV::operator^=(const FddNodeV& n) {
    (*this) = (*this) ^ n;
    return (*this);
}

FddNodeV
FddNodeV::operator&(const FddNodeV& n) const {
    return _FddMgrV->fddAnd((*this), n);
}

FddNodeV&
FddNodeV::operator&=(const FddNodeV& n) {
    (*this) = (*this) & n;
    return (*this);
}

// a | b = (a ^ b) ^ (a & b)
FddNodeV
FddNodeV::operator|(const FddNodeV& n) const {
    FddNodeV x = (*this) ^ n;
    FddNodeV a = (*this) & n;
    return x ^ a;
}

FddNodeV&
FddNodeV::operator|=(const FddNodeV& n) {
    (*this) = (*this) | n;
    return (*this);
}

bool FddNodeV::operator<(const FddNodeV& n) const {
    unsigned l1 = getLevel();
    unsigned l2 = n.getLevel();
    return ((l1 < l2) || ((l1 == l2) && (_nodeV < n._nodeV)));
}

bool FddNodeV::operator<=(const FddNodeV& n) const {
    unsigned l1 = getLevel();
    unsigned l2 = n.getLevel();
    return ((l1 < l2) || ((l1 == l2) && (_nodeV <= n._nodeV)));
}

bool FddNodeV::isTerminal() const {
    return (getFddNodeVInt() == FddNodeVInt::_terminal);
}

ostream&
operator<<(ostream& os, const FddNodeV& n) {
    size_t nNodes = 0;
    n.print(os, 0, nNodes);
    n.unsetVisitedRecur();
    os << endl
       << endl
       << "==> Total #FddNodeVs : " << nNodes << endl;
    return os;
}

void FddNodeV::print(ostream& os, size_t indent, size_t& nNodes) const {
    for (size_t i = 0; i < indent; ++i)
        os << ' ';
    FddNodeVInt* n = getFddNodeVInt();
    os << '[' << getLevel() << "](" << (isNegEdge() ? '-' : '+') << ") ";
    if (_debugFddAddr)
        os << n << " ";
    if (_debugRefCount)
        os << "(" << n->getRefCount() << ")";
    if (n->isVisited()) {
        os << " (*)";
        return;
    } else
        ++nNodes;
    n->setVisited();
    if (!isTerminal()) {
        os << endl;
        n->getLeft().print(os, indent + 2, nNodes);
        os << endl;
        n->getRight().print(os, indent + 2, nNodes);
    }
}

void FddNodeV::unsetVisitedRecur() const {
    FddNodeVInt* n = getFddNodeVInt();
    if (!n->isVisited()) return;
    n->unsetVisited();
    if (!isTerminal()) {
        n->getLeft().unsetVisitedRecur();
        n->getRight().unsetVisitedRecur();
    }
}

void FddNodeV::drawFdd(const string& name, ofstream& ofile) const {
    ofile << "digraph {" << endl;
    ofile << "   node [shape = plaintext];" << endl;
    ofile << "   ";
    for (unsigned l = getLevel(); l > 0; --l)
        ofile << l << " -> ";
    ofile << "0 [style = invis];" << endl;
    ofile << "   { rank = source; \"" << name << "\"; }" << endl;
    ofile << "   node [shape = ellipse];" << endl;
    ofile << "   \"" << name << "\" -> \"" << getLabel()
          << "\" [color = blue]";
    ofile << (isNegEdge() ? " [arrowhead = odot]" : ";") << endl;

    drawFddRecur(ofile);

    ofile << "   { rank = same; 0; \"One\"; }" << endl;
    ofile << "}" << endl;

    unsetVisitedRecur();
}

void FddNodeV::drawFddRecur(ofstream& ofile) const {
    FddNodeVInt* n = getFddNodeVInt();
    if (n->isVisited()) return;
    n->setVisited();
    if (isTerminal()) return;
    FddNodeV left  = getLeft();
    FddNodeV right = getRight();

    ofile << "   { rank = same; " << getLevel() << "; \"" << getLabel()
          << "\"; }\n";

    // Left edge = fx_bar (negative cofactor), solid line
    ofile << "   \"" << getLabel() << "\" -> \"" << left.getLabel() << "\"";
    ofile << ((left.isNegEdge()) ? " [arrowhead=odot]" : ";") << endl;

    // Right edge = fd (Boolean difference), dotted red line
    ofile << "   \"" << getLabel() << "\" -> \"" << right.getLabel()
          << "\"[style = dotted ] [color=red]";
    ofile << ((right.isNegEdge()) ? " [arrowhead=odot]" : ";") << endl;

    left.drawFddRecur(ofile);
    right.drawFddRecur(ofile);
}

string
FddNodeV::getLabel() const {
    if (isTerminal()) return "One";
    ostringstream str;
    str << getFddNodeVInt();
    return str.str();
}
