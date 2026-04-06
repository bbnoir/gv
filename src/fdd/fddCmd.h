/****************************************************************************
  FileName     [ fddCmd.h ]
  PackageName  [ fdd ]
  Synopsis     [ Define classes for FDD commands ]
  Author       [ ]
  Copyright    [ Copyright(c) 2023-present DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#pragma once

#include "gvCmdMgr.h"

GV_COMMAND(FResetCmd, GV_CMD_TYPE_FDD);
GV_COMMAND(FSetVarCmd, GV_CMD_TYPE_FDD);
GV_COMMAND(FInvCmd, GV_CMD_TYPE_FDD);
GV_COMMAND(FAndCmd, GV_CMD_TYPE_FDD);
GV_COMMAND(FOrCmd, GV_CMD_TYPE_FDD);
GV_COMMAND(FNandCmd, GV_CMD_TYPE_FDD);
GV_COMMAND(FNorCmd, GV_CMD_TYPE_FDD);
GV_COMMAND(FXorCmd, GV_CMD_TYPE_FDD);
GV_COMMAND(FXnorCmd, GV_CMD_TYPE_FDD);
GV_COMMAND(FCompareCmd, GV_CMD_TYPE_FDD);
GV_COMMAND(FSimulateCmd, GV_CMD_TYPE_FDD);
GV_COMMAND(FReportCmd, GV_CMD_TYPE_FDD);
GV_COMMAND(FDrawCmd, GV_CMD_TYPE_FDD);
GV_COMMAND(FSetOrderCmd, GV_CMD_TYPE_FDD);
GV_COMMAND(FConstructCmd, GV_CMD_TYPE_FDD);
