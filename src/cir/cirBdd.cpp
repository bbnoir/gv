/****************************************************************************
  FileName     [ cirBdd.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define BDD manager functions ]
  Author       [ Design Verification Lab ]
  Copyright    [ Copyright(c) 2023-present DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#include <algorithm>
#include <unordered_set>

#include "bddMgrV.h"   // MODIFICATION FOR SoCV BDD
#include "bddNodeV.h"  // MODIFICATION FOR SoCV BDD
#include "cirGate.h"
#include "cirMgr.h"
#include "gvMsg.h"
#include "util.h"

extern BddMgrV* bddMgrV;  // MODIFICATION FOR SoCV BDD

namespace gv {
namespace cir {

const bool CirMgr::setBddOrder(BddVarOrder order) {
    unsigned supportSize = getNumPIs() + 2 * getNumLATCHs();
    if (supportSize >= bddMgrV->getNumSupports()) {
        gvMsg(GV_MSG_ERR) << "BDD Support Size is Smaller Than Current Design Required !!" << endl;
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
        // Order follows CirMgr::_dfsList: post-order DFS from each PO then each RI
        // (FF input); first occurrence of each support gate defines variable order.
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
        bddMgrV->addBddNodeV(gate->getGid(), bddMgrV->getSupport(supportId)());
        ++supportId;
    }
    for (CirRoGate* gate : roOrder) {
        bddMgrV->addBddNodeV(gate->getGid(), bddMgrV->getSupport(supportId)());
        ++supportId;
    }
    for (CirRiGate* gate : riOrder) {
        bddMgrV->addBddNodeV(gate->getName(), bddMgrV->getSupport(supportId)());
        ++supportId;
    }
    bddMgrV->addBddNodeV(_const0->getGid(), BddNodeV::_zero());
    ++supportId;

    return true;
}

void CirMgr::buildNtkBdd() {
    // TODO: build BDD for ntk here
    // Perform DFS traversal from DFF inputs, inout, and output gates.
    // Collect ordered nets to a GVNetVec
    // Construct BDDs in the DFS order

    // build PO
    for (unsigned i = 0; i < getNumPOs(); ++i) {
        buildBdd(getPo(i));
    }

    // build next state (RI)
    for (unsigned i = 0; i < getNumLATCHs(); ++i) {
        CirGate* left = getRi(i);  // get RI
        if (bddMgrV->getBddNodeV(left->getGid()) == (size_t)0) {
            buildBdd(left);
        }
    }
}

void CirMgr::buildBdd(CirGate* gate) {
    GateVec orderedGates;
    clearList(orderedGates);
    CirGate::setGlobalRef();
    gate->genDfsList(orderedGates);
    assert(orderedGates.size() <= getNumTots());

    // TODO: build BDD for the specified net here
    CirGateV left;
    CirGateV right;
    for (unsigned i = 0; i < orderedGates.size(); ++i) {
        if (orderedGates[i]->getType() == AIG_GATE) {
            // build fanin
            left             = orderedGates[i]->getIn0();
            right            = orderedGates[i]->getIn1();
            BddNodeV newNode = ((left.isInv()) ? ~bddMgrV->getBddNodeV(left.gateId())
                                               : bddMgrV->getBddNodeV(left.gateId())) &
                               ((right.isInv()) ? ~bddMgrV->getBddNodeV(right.gateId())
                                                : bddMgrV->getBddNodeV(right.gateId()));
            bddMgrV->addBddNodeV(orderedGates[i]->getGid(), newNode());
        }
        // PO, RI
        else if ((orderedGates[i]->getType() == RI_GATE) || (orderedGates[i]->getType() == PO_GATE)) {
            CirGateV in0     = orderedGates[i]->getIn0();
            BddNodeV newNode = (in0.isInv()) ? ~bddMgrV->getBddNodeV(in0.gateId()) : bddMgrV->getBddNodeV(in0.gateId());
            bddMgrV->addBddNodeV(orderedGates[i]->getGid(), newNode());
        }
    }
}

}  // namespace cir
}  // namespace gv
