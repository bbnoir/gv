/****************************************************************************
  FileName     [ cirFdd.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define FDD manager functions for circuit construction ]
  Author       [ ]
  Copyright    [ Copyright(c) 2023-present DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#include <algorithm>
#include <unordered_set>

#include "cirGate.h"
#include "cirMgr.h"
#include "fddMgrV.h"
#include "fddNodeV.h"
#include "gvMsg.h"
#include "util.h"

extern FddMgrV* fddMgrV;

namespace gv {
namespace cir {

const bool CirMgr::setFddOrder(BddVarOrder order) {
    unsigned supportSize = getNumPIs() + 2 * getNumLATCHs();
    if (supportSize >= fddMgrV->getNumSupports()) {
        gvMsg(GV_MSG_ERR) << "FDD Support Size is Smaller Than Current Design Required !!" << endl;
        return false;
    }

    PiVec piOrder;
    RoVec roOrder;
    RiVec riOrder;

    const bool useFile = (order == BddVarOrder::File || order == BddVarOrder::RFile);
    if (useFile) {
        for (unsigned i = 0, n = getNumPIs(); i < n; ++i)
            piOrder.push_back(getPi(i));
        for (unsigned i = 0, n = getNumLATCHs(); i < n; ++i)
            roOrder.push_back(getRo(i));
        for (unsigned i = 0, n = getNumLATCHs(); i < n; ++i)
            riOrder.push_back(getRi(i));
        if (order == BddVarOrder::RFile) {
            reverse(piOrder.begin(), piOrder.end());
            reverse(roOrder.begin(), roOrder.end());
            reverse(riOrder.begin(), riOrder.end());
        }
    } else {
        unordered_set<unsigned> seenPi, seenRo, seenRi;
        for (CirGate* g : _dfsList) {
            const GateType t = g->getType();
            if (t == PI_GATE) {
                const unsigned id = g->getGid();
                if (seenPi.insert(id).second)
                    piOrder.push_back(static_cast<CirPiGate*>(g));
            } else if (t == RO_GATE) {
                const unsigned id = g->getGid();
                if (seenRo.insert(id).second)
                    roOrder.push_back(static_cast<CirRoGate*>(g));
            } else if (t == RI_GATE) {
                const unsigned id = g->getGid();
                if (seenRi.insert(id).second)
                    riOrder.push_back(static_cast<CirRiGate*>(g));
            }
        }
        for (unsigned i = 0, n = getNumPIs(); i < n; ++i) {
            CirPiGate* p = getPi(i);
            if (!seenPi.count(p->getGid())) piOrder.push_back(p);
        }
        for (unsigned i = 0, n = getNumLATCHs(); i < n; ++i) {
            CirRoGate* r = getRo(i);
            if (!seenRo.count(r->getGid())) roOrder.push_back(r);
        }
        for (unsigned i = 0, n = getNumLATCHs(); i < n; ++i) {
            CirRiGate* r = getRi(i);
            if (!seenRi.count(r->getGid())) riOrder.push_back(r);
        }
        if (order == BddVarOrder::RDfs) {
            reverse(piOrder.begin(), piOrder.end());
            reverse(roOrder.begin(), roOrder.end());
            reverse(riOrder.begin(), riOrder.end());
        }
    }

    unsigned supportId = 1;
    for (CirPiGate* gate : piOrder) {
        fddMgrV->addFddNodeV(gate->getGid(), fddMgrV->getSupport(supportId)());
        ++supportId;
    }
    for (CirRoGate* gate : roOrder) {
        fddMgrV->addFddNodeV(gate->getGid(), fddMgrV->getSupport(supportId)());
        ++supportId;
    }
    for (CirRiGate* gate : riOrder) {
        fddMgrV->addFddNodeV(gate->getName(), fddMgrV->getSupport(supportId)());
        ++supportId;
    }
    fddMgrV->addFddNodeV(_const0->getGid(), FddNodeV::_zero());
    ++supportId;

    return true;
}

void CirMgr::buildNtkFdd() {
    for (unsigned i = 0; i < getNumPOs(); ++i) {
        buildFdd(getPo(i));
    }
    for (unsigned i = 0; i < getNumLATCHs(); ++i) {
        CirGate* left = getRi(i);
        if (fddMgrV->getFddNodeV(left->getGid()) == (size_t)0) {
            buildFdd(left);
        }
    }
}

void CirMgr::buildFdd(CirGate* gate) {
    GateVec orderedGates;
    clearList(orderedGates);
    CirGate::setGlobalRef();
    gate->genDfsList(orderedGates);
    assert(orderedGates.size() <= getNumTots());

    CirGateV left;
    CirGateV right;
    for (unsigned i = 0; i < orderedGates.size(); ++i) {
        if (orderedGates[i]->getType() == AIG_GATE) {
            left  = orderedGates[i]->getIn0();
            right = orderedGates[i]->getIn1();
            FddNodeV newNode = ((left.isInv()) ? ~fddMgrV->getFddNodeV(left.gateId())
                                               : fddMgrV->getFddNodeV(left.gateId())) &
                               ((right.isInv()) ? ~fddMgrV->getFddNodeV(right.gateId())
                                                : fddMgrV->getFddNodeV(right.gateId()));
            fddMgrV->addFddNodeV(orderedGates[i]->getGid(), newNode());
        } else if ((orderedGates[i]->getType() == RI_GATE) || (orderedGates[i]->getType() == PO_GATE)) {
            CirGateV in0     = orderedGates[i]->getIn0();
            FddNodeV newNode = (in0.isInv()) ? ~fddMgrV->getFddNodeV(in0.gateId()) : fddMgrV->getFddNodeV(in0.gateId());
            fddMgrV->addFddNodeV(orderedGates[i]->getGid(), newNode());
        }
    }
}

}  // namespace cir
}  // namespace gv
