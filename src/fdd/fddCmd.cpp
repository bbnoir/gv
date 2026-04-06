/****************************************************************************
  FileName     [ fddCmd.cpp ]
  PackageName  [ fdd ]
  Synopsis     [ Define FDD commands ]
  Author       [ ]
  Copyright    [ Copyright(c) 2023-present DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#include "fddCmd.h"

#include <cassert>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "cirGate.h"
#include "cirMgr.h"
#include "fddMgrV.h"
#include "gvMsg.h"
#include "util.h"

using namespace gv::cir;
using namespace std;

bool initFddCmd() {
    if (fddMgrV) delete fddMgrV;
    fddMgrV = new FddMgrV;
    return (gvCmdMgr->regCmd("FRESET", 6, new FResetCmd) &&
            gvCmdMgr->regCmd("FSETVar", 5, new FSetVarCmd) &&
            gvCmdMgr->regCmd("FINV", 4, new FInvCmd) &&
            gvCmdMgr->regCmd("FAND", 4, new FAndCmd) &&
            gvCmdMgr->regCmd("FOr", 3, new FOrCmd) &&
            gvCmdMgr->regCmd("FNAND", 5, new FNandCmd) &&
            gvCmdMgr->regCmd("FNOR", 4, new FNorCmd) &&
            gvCmdMgr->regCmd("FXOR", 4, new FXorCmd) &&
            gvCmdMgr->regCmd("FXNOR", 4, new FXnorCmd) &&
            gvCmdMgr->regCmd("FCOMpare", 4, new FCompareCmd) &&
            gvCmdMgr->regCmd("FSIMulate", 4, new FSimulateCmd) &&
            gvCmdMgr->regCmd("FREPort", 4, new FReportCmd) &&
            gvCmdMgr->regCmd("FDRAW", 5, new FDrawCmd) &&
            gvCmdMgr->regCmd("FSETOrder", 5, new FSetOrderCmd) &&
            gvCmdMgr->regCmd("FCONstruct", 4, new FConstructCmd));
}

static bool isValidFddName(const string& str) {
    int id;
    return (isValidVarName(str) || (myStr2Int(str, id) && id >= 0));
}

extern FddNodeV getFddNodeV(const string& fddName);
bool setFddOrder_flag = false;

//----------------------------------------------------------------------
//    FRESET <(size_t nSupports)> <(size_t hashSize)> <(size_t cacheSize)>
//----------------------------------------------------------------------
GVCmdExecStatus
FResetCmd::exec(const string& option) {
    vector<string> options;
    GVCmdExec::lexOptions(option, options);
    if (options.size() < 3)
        return GVCmdExec::errorOption(GV_CMD_OPT_MISSING, "");
    if (options.size() > 3)
        return GVCmdExec::errorOption(GV_CMD_OPT_EXTRA, options[3]);

    int nSupports, hashSize, cacheSize;
    if (!myStr2Int(options[0], nSupports) || (nSupports <= 0))
        return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[0]);
    if (!myStr2Int(options[1], hashSize) || (hashSize <= 0))
        return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[1]);
    if (!myStr2Int(options[2], cacheSize) || (cacheSize <= 0))
        return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[2]);

    assert(fddMgrV != 0);
    fddMgrV->init(nSupports, hashSize, cacheSize);
    return GV_CMD_EXEC_DONE;
}

void FResetCmd::usage(const bool& verbose) const {
    cout << "Usage: FRESET <(size_t nSupports)> <(size_t hashSize)> "
         << "<(size_t cacheSize)>" << endl;
}

void FResetCmd::help() const {
    cout << setw(20) << left << "FRESET: "
         << "FDD reset" << endl;
}

//----------------------------------------------------------------------
//    FSETVar <(size_t level)> <(string varName)>
//----------------------------------------------------------------------
GVCmdExecStatus
FSetVarCmd::exec(const string& option) {
    vector<string> options;
    GVCmdExec::lexOptions(option, options);
    if (options.size() < 2)
        return GVCmdExec::errorOption(GV_CMD_OPT_MISSING, "");
    if (options.size() > 2)
        return GVCmdExec::errorOption(GV_CMD_OPT_EXTRA, options[2]);

    int level;
    if (myStr2Int(options[0], level) && (level >= 1) &&
        (size_t(level) < fddMgrV->getNumSupports())) {
        FddNodeV n = fddMgrV->getSupport(level);
        if (!isValidVarName(options[1]) ||
            !fddMgrV->addFddNodeV(options[1], n()))
            return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[1]);
        return GV_CMD_EXEC_DONE;
    } else
        return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[0]);
}

void FSetVarCmd::usage(const bool& verbose) const {
    cout << "Usage: FSETVar <(size_t level)> <(string varName)>" << endl;
}

void FSetVarCmd::help() const {
    cout << setw(20) << left << "FSETVar: "
         << "FDD set a variable name for a support" << endl;
}

//----------------------------------------------------------------------
//    FINV <(string varName)> <(string fddName)>
//----------------------------------------------------------------------
GVCmdExecStatus
FInvCmd::exec(const string& option) {
    vector<string> options;
    GVCmdExec::lexOptions(option, options);
    if (options.size() < 2)
        return GVCmdExec::errorOption(GV_CMD_OPT_MISSING, "");
    if (options.size() > 2)
        return GVCmdExec::errorOption(GV_CMD_OPT_EXTRA, options[2]);

    if (!isValidVarName(options[0]))
        return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[0]);
    if (!isValidFddName(options[1]))
        return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[1]);
    FddNodeV b = ::getFddNodeV(options[1]);
    if (b() == 0) return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[1]);
    fddMgrV->forceAddFddNodeV(options[0], (~b)());
    return GV_CMD_EXEC_DONE;
}

void FInvCmd::usage(const bool& verbose) const {
    cout << "Usage: FINV <(string varName)> <(string fddName)>" << endl;
}

void FInvCmd::help() const {
    cout << setw(20) << left << "FINV: "
         << "FDD Inv" << endl;
}

//----------------------------------------------------------------------
//    FAND <(string varName)> <(string fddName)>...
//----------------------------------------------------------------------
GVCmdExecStatus
FAndCmd::exec(const string& option) {
    vector<string> options;
    GVCmdExec::lexOptions(option, options);
    size_t n = options.size();
    if (n < 2) return GVCmdExec::errorOption(GV_CMD_OPT_MISSING, "");

    if (!isValidVarName(options[0]))
        return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[0]);

    FddNodeV ret = FddNodeV::_one;
    for (size_t i = 1; i < n; ++i) {
        if (!isValidFddName(options[i]))
            return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[i]);
        FddNodeV b = ::getFddNodeV(options[i]);
        if (b() == 0)
            return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[i]);
        ret &= b;
    }
    fddMgrV->forceAddFddNodeV(options[0], ret());
    return GV_CMD_EXEC_DONE;
}

void FAndCmd::usage(const bool& verbose) const {
    cout << "Usage: FAND <(string varName)> <(string fddName)>..." << endl;
}

void FAndCmd::help() const {
    cout << setw(20) << left << "FAND: "
         << "FDD And" << endl;
}

//----------------------------------------------------------------------
//    FOR <(string varName)> <(string fddName)>...
//----------------------------------------------------------------------
GVCmdExecStatus
FOrCmd::exec(const string& option) {
    vector<string> options;
    GVCmdExec::lexOptions(option, options);
    size_t n = options.size();
    if (n < 2) return GVCmdExec::errorOption(GV_CMD_OPT_MISSING, "");

    if (!isValidVarName(options[0]))
        return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[0]);

    FddNodeV ret = FddNodeV::_zero;
    for (size_t i = 1; i < n; ++i) {
        if (!isValidFddName(options[i]))
            return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[i]);
        FddNodeV b = ::getFddNodeV(options[i]);
        if (b() == 0)
            return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[i]);
        ret |= b;
    }
    fddMgrV->forceAddFddNodeV(options[0], ret());
    return GV_CMD_EXEC_DONE;
}

void FOrCmd::usage(const bool& verbose) const {
    cout << "Usage: FOR <(string varName)> <(string fddName)>..." << endl;
}

void FOrCmd::help() const {
    cout << setw(20) << left << "FOR: "
         << "FDD Or" << endl;
}

//----------------------------------------------------------------------
//    FNAND <(string varName)> <(string fddName)>...
//----------------------------------------------------------------------
GVCmdExecStatus
FNandCmd::exec(const string& option) {
    vector<string> options;
    GVCmdExec::lexOptions(option, options);
    size_t n = options.size();
    if (n < 2) return GVCmdExec::errorOption(GV_CMD_OPT_MISSING, "");

    if (!isValidVarName(options[0]))
        return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[0]);

    FddNodeV ret = FddNodeV::_one;
    for (size_t i = 1; i < n; ++i) {
        if (!isValidFddName(options[i]))
            return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[i]);
        FddNodeV b = ::getFddNodeV(options[i]);
        if (b() == 0)
            return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[i]);
        ret &= b;
    }
    ret = ~ret;
    fddMgrV->forceAddFddNodeV(options[0], ret());
    return GV_CMD_EXEC_DONE;
}

void FNandCmd::usage(const bool& verbose) const {
    cout << "Usage: FNAND <(string varName)> <(string fddName)>..." << endl;
}

void FNandCmd::help() const {
    cout << setw(20) << left << "FNAND: "
         << "FDD Nand" << endl;
}

//----------------------------------------------------------------------
//    FNOR <(string varName)> <(string fddName)>...
//----------------------------------------------------------------------
GVCmdExecStatus
FNorCmd::exec(const string& option) {
    vector<string> options;
    GVCmdExec::lexOptions(option, options);
    size_t n = options.size();
    if (n < 2) return GVCmdExec::errorOption(GV_CMD_OPT_MISSING, "");

    if (!isValidVarName(options[0]))
        return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[0]);

    FddNodeV ret = FddNodeV::_zero;
    for (size_t i = 1; i < n; ++i) {
        if (!isValidFddName(options[i]))
            return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[i]);
        FddNodeV b = ::getFddNodeV(options[i]);
        if (b() == 0)
            return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[i]);
        ret |= b;
    }
    ret = ~ret;
    fddMgrV->forceAddFddNodeV(options[0], ret());
    return GV_CMD_EXEC_DONE;
}

void FNorCmd::usage(const bool& verbose) const {
    cout << "Usage: FNOR <(string varName)> <(string fddName)>..." << endl;
}

void FNorCmd::help() const {
    cout << setw(20) << left << "FNOR: "
         << "FDD Nor" << endl;
}

//----------------------------------------------------------------------
//    FXOR <(string varName)> <(string fddName)>...
//----------------------------------------------------------------------
GVCmdExecStatus
FXorCmd::exec(const string& option) {
    vector<string> options;
    GVCmdExec::lexOptions(option, options);
    size_t n = options.size();
    if (n < 2) return GVCmdExec::errorOption(GV_CMD_OPT_MISSING, "");

    if (!isValidVarName(options[0]))
        return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[0]);

    FddNodeV ret = FddNodeV::_zero;
    for (size_t i = 1; i < n; ++i) {
        if (!isValidFddName(options[i]))
            return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[i]);
        FddNodeV b = ::getFddNodeV(options[i]);
        if (b() == 0)
            return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[i]);
        ret ^= b;
    }
    fddMgrV->forceAddFddNodeV(options[0], ret());
    return GV_CMD_EXEC_DONE;
}

void FXorCmd::usage(const bool& verbose) const {
    cout << "Usage: FXOR <(string varName)> <(string fddName)>..." << endl;
}

void FXorCmd::help() const {
    cout << setw(20) << left << "FXOR: "
         << "FDD Xor" << endl;
}

//----------------------------------------------------------------------
//    FXNOR <(string varName)> <(string fddName)>...
//----------------------------------------------------------------------
GVCmdExecStatus
FXnorCmd::exec(const string& option) {
    vector<string> options;
    GVCmdExec::lexOptions(option, options);
    size_t n = options.size();
    if (n < 2) return GVCmdExec::errorOption(GV_CMD_OPT_MISSING, "");

    if (!isValidVarName(options[0]))
        return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[0]);

    FddNodeV ret = FddNodeV::_zero;
    for (size_t i = 1; i < n; ++i) {
        if (!isValidFddName(options[i]))
            return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[i]);
        FddNodeV b = ::getFddNodeV(options[i]);
        if (b() == 0)
            return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[i]);
        ret ^= b;
    }
    ret = ~ret;
    fddMgrV->forceAddFddNodeV(options[0], ret());
    return GV_CMD_EXEC_DONE;
}

void FXnorCmd::usage(const bool& verbose) const {
    cout << "Usage: FXNOR <(string varName)> <(string fddName)>..." << endl;
}

void FXnorCmd::help() const {
    cout << setw(20) << left << "FXNOR: "
         << "FDD Xnor" << endl;
}

//----------------------------------------------------------------------
//    FCOMpare <(string fddName)> <(string fddName)>
//----------------------------------------------------------------------
GVCmdExecStatus
FCompareCmd::exec(const string& option) {
    vector<string> options;
    GVCmdExec::lexOptions(option, options);
    if (options.size() < 2)
        return GVCmdExec::errorOption(GV_CMD_OPT_MISSING, "");
    if (options.size() > 2)
        return GVCmdExec::errorOption(GV_CMD_OPT_EXTRA, options[2]);

    if (!isValidFddName(options[0]))
        return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[0]);
    FddNodeV b0 = ::getFddNodeV(options[0]);
    if (b0() == 0)
        return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[0]);

    if (!isValidFddName(options[1]))
        return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[1]);
    FddNodeV b1 = ::getFddNodeV(options[1]);
    if (b1() == 0)
        return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[1]);

    if (b0 == b1)
        cout << "\"" << options[0] << "\" and \"" << options[1]
             << "\" are equivalent." << endl;
    else if (b0 == ~b1)
        cout << "\"" << options[0] << "\" and \"" << options[1]
             << "\" are inversely equivalent." << endl;
    else
        cout << "\"" << options[0] << "\" and \"" << options[1]
             << "\" are not equivalent." << endl;
    return GV_CMD_EXEC_DONE;
}

void FCompareCmd::usage(const bool& verbose) const {
    cout << "Usage: FCOMpare <(string fddName)> <(string fddName)>" << endl;
}

void FCompareCmd::help() const {
    cout << setw(20) << left << "FCOMpare: "
         << "FDD comparison" << endl;
}

//----------------------------------------------------------------------
//    FSIMulate <(string fddName)> <(bit_string inputPattern)>
//----------------------------------------------------------------------
GVCmdExecStatus
FSimulateCmd::exec(const string& option) {
    vector<string> options;
    GVCmdExec::lexOptions(option, options);
    if (options.size() < 2)
        return GVCmdExec::errorOption(GV_CMD_OPT_MISSING, "");
    if (options.size() > 2)
        return GVCmdExec::errorOption(GV_CMD_OPT_EXTRA, options[2]);

    if (!isValidFddName(options[0]))
        return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[0]);
    FddNodeV node = ::getFddNodeV(options[0]);
    if (node() == 0)
        return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[0]);

    int value = fddMgrV->evalCube(node, options[1]);
    if (value == -1)
        return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[1]);

    cout << "FDD Simulate: " << options[1] << " = " << value << endl;
    return GV_CMD_EXEC_DONE;
}

void FSimulateCmd::usage(const bool& verbose) const {
    cout << "Usage: FSIMulate <(string fddName)> <(bit_string inputPattern)>"
         << endl;
}

void FSimulateCmd::help() const {
    cout << setw(20) << left << "FSIMulate: "
         << "FDD simulation" << endl;
}

//----------------------------------------------------------------------
//    FREPort <(string fddName)> [-ADDRess] [-REFcount]
//            [-File <(string fileName)>]
//----------------------------------------------------------------------
GVCmdExecStatus
FReportCmd::exec(const string& option) {
    vector<string> options;
    GVCmdExec::lexOptions(option, options);

    if (options.empty()) return GVCmdExec::errorOption(GV_CMD_OPT_MISSING, "");

    bool doFile = false, doAddr = false, doRefCount = false;
    string fddNodeVName, fileName;
    FddNodeV fnode;
    for (size_t i = 0, n = options.size(); i < n; ++i) {
        if (myStrNCmp("-File", options[i], 2) == 0) {
            if (doFile)
                return GVCmdExec::errorOption(GV_CMD_OPT_EXTRA, options[i]);
            if (++i == n)
                return GVCmdExec::errorOption(GV_CMD_OPT_MISSING, options[i - 1]);
            fileName = options[i];
            doFile   = true;
        } else if (myStrNCmp("-ADDRess", options[i], 5) == 0) {
            if (doAddr)
                return GVCmdExec::errorOption(GV_CMD_OPT_EXTRA, options[i]);
            doAddr = true;
        } else if (myStrNCmp("-REFcount", options[i], 4) == 0) {
            if (doRefCount)
                return GVCmdExec::errorOption(GV_CMD_OPT_EXTRA, options[i]);
            doRefCount = true;
        } else if (fddNodeVName.size())
            return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[i]);
        else {
            fddNodeVName = options[i];
            if (!isValidFddName(fddNodeVName))
                return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, fddNodeVName);
            fnode = ::getFddNodeV(fddNodeVName);
            if (fnode() == 0)
                return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, fddNodeVName);
        }
    }

    if (!fddNodeVName.size())
        return GVCmdExec::errorOption(GV_CMD_OPT_MISSING, "");
    if (doAddr) FddNodeV::_debugFddAddr = true;
    if (doRefCount) FddNodeV::_debugRefCount = true;
    if (doFile) {
        ofstream ofs(fileName.c_str());
        if (!ofs)
            return GVCmdExec::errorOption(GV_CMD_OPT_FOPEN_FAIL, fileName);
        ofs << fnode << endl;
    } else
        cout << fnode << endl;

    FddNodeV::_debugFddAddr  = false;
    FddNodeV::_debugRefCount = false;
    return GV_CMD_EXEC_DONE;
}

void FReportCmd::usage(const bool& verbose) const {
    cout << "Usage: FREPort <(string fddName)> [-ADDRess] [-REFcount]\n"
         << "               [-File <(string fileName)>]" << endl;
}

void FReportCmd::help() const {
    cout << setw(20) << left << "FREPort: "
         << "FDD report node" << endl;
}

//----------------------------------------------------------------------
//    FDRAW <(string fddName)> <(string fileName)>
//----------------------------------------------------------------------
GVCmdExecStatus
FDrawCmd::exec(const string& option) {
    vector<string> options;
    GVCmdExec::lexOptions(option, options);
    if (options.size() < 2)
        return GVCmdExec::errorOption(GV_CMD_OPT_MISSING, "");
    if (options.size() > 2)
        return GVCmdExec::errorOption(GV_CMD_OPT_EXTRA, options[2]);

    if (!isValidFddName(options[0]))
        return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[0]);
    if (::getFddNodeV(options[0])() == 0)
        return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[0]);
    if (!fddMgrV->drawFdd(options[0], options[1]))
        return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[1]);
    return GV_CMD_EXEC_DONE;
}

void FDrawCmd::usage(const bool& verbose) const {
    cout << "Usage: FDRAW <(string fddName)> <(string fileName)>" << endl;
}

void FDrawCmd::help() const {
    cout << setw(20) << left << "FDRAW: "
         << "FDD graphic draw" << endl;
}

//----------------------------------------------------------------------
//    FSETOrder < -File | -RFile | -DFS | -RDFS >
//----------------------------------------------------------------------
GVCmdExecStatus
FSetOrderCmd::exec(const string& option) {
    if (setFddOrder_flag) {
        gvMsg(GV_MSG_WAR) << "FDD Variable Order Has Been Set !!" << endl;
        return GV_CMD_EXEC_ERROR;
    }
    vector<string> options;
    GVCmdExec::lexOptions(option, options);
    if (options.size() < 1)
        return GVCmdExec::errorOption(GV_CMD_OPT_MISSING, "");
    if (options.size() > 1)
        return GVCmdExec::errorOption(GV_CMD_OPT_EXTRA, options[1]);

    string token = options[0];
    BddVarOrder ord;
    if (myStrNCmp("-File", token, 2) == 0)
        ord = BddVarOrder::File;
    else if (myStrNCmp("-RFile", token, 3) == 0)
        ord = BddVarOrder::RFile;
    else if (myStrNCmp("-RDFS", token, 3) == 0)
        ord = BddVarOrder::RDfs;
    else if (myStrNCmp("-DFS", token, 2) == 0)
        ord = BddVarOrder::Dfs;
    else
        return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, token);

    fddMgrV->restart();
    setFddOrder_flag = cirMgr->setFddOrder(ord);
    if (!setFddOrder_flag)
        gvMsg(GV_MSG_ERR) << "Set FDD Variable Order Failed !!" << endl;
    else
        cout << "Set FDD Variable Order Succeed !!" << endl;
    return GV_CMD_EXEC_DONE;
}

void FSetOrderCmd::usage(const bool& verbose) const {
    cout << "Usage: FSETOrder < -File | -RFile | -DFS | -RDFS >" << endl;
}

void FSetOrderCmd::help() const {
    cout << setw(20) << left << "FSETOrder: "
         << "Set FDD variable Order From Circuit." << endl;
}

//----------------------------------------------------------------------
//    FConstruct <-Gateid <gateId> | -Output <outputIndex> | -All>
//----------------------------------------------------------------------
GVCmdExecStatus
FConstructCmd::exec(const string& option) {
    if (!setFddOrder_flag) {
        gvMsg(GV_MSG_WAR) << "FDD variable order has not been set !!!" << endl;
        return GV_CMD_EXEC_ERROR;
    }

    vector<string> options;
    GVCmdExec::lexOptions(option, options);
    if (options.size() < 1)
        return GVCmdExec::errorOption(GV_CMD_OPT_MISSING, "");
    if (options.size() > 2)
        return GVCmdExec::errorOption(GV_CMD_OPT_EXTRA, options[2]);

    bool isGate = false, isOutput = false;
    if (myStrNCmp("-All", options[0], 2) == 0)
        cirMgr->buildNtkFdd();
    else if (myStrNCmp("-Gateid", options[0], 2) == 0)
        isGate = true;
    else if (myStrNCmp("-Output", options[0], 2) == 0)
        isOutput = true;
    else
        return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[0]);

    if (isOutput || isGate) {
        if (options.size() != 2)
            return GVCmdExec::errorOption(GV_CMD_OPT_MISSING, options[0]);
        int num = 0;
        CirGate* gate;
        if (!myStr2Int(options[1], num) || (num < 0))
            return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[1]);
        if (isGate) {
            if ((unsigned)num >= cirMgr->getNumTots()) {
                gvMsg(GV_MSG_ERR) << "Gate with Id " << num << " does NOT Exist in Current Cir !!" << endl;
                return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[1]);
            }
            gate = cirMgr->getGate(num);
        } else if (isOutput) {
            if ((unsigned)num >= cirMgr->getNumPOs()) {
                gvMsg(GV_MSG_ERR) << "Output with Index " << num << " does NOT Exist in Current Cir !!" << endl;
                return GVCmdExec::errorOption(GV_CMD_OPT_ILLEGAL, options[1]);
            }
            gate = cirMgr->getPo(num);
        }
        cirMgr->buildFdd(gate);
    }
    return GV_CMD_EXEC_DONE;
}

void FConstructCmd::usage(const bool& verbose) const {
    cout << "Usage: FConstruct <-Gateid <gateId> | -Output <outputIndex> | -All > "
         << endl;
}

void FConstructCmd::help() const {
    cout << setw(20) << left << "FConstruct: "
         << "Build FDD From Current Design." << endl;
}
