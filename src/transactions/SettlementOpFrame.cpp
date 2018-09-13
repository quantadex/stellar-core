// Copyright 2014 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "util/asio.h"
#include "transactions/SettlementOpFrame.h"
#include "database/Database.h"
#include "ledger/DataFrame.h"
#include "ledger/LedgerDelta.h"
#include "main/Application.h"
#include "medida/meter.h"
#include "medida/metrics_registry.h"
#include "util/Logging.h"
#include "util/types.h"

namespace stellar
{

using namespace std;
using xdr::operator==;

SettlementOpFrame::SettlementOpFrame(Operation const& op, OperationResult& res,
                                     TransactionFrame& parentTx)
    : OperationFrame(op, res, parentTx),
      mSettlement(mOperation.body.settlementOp())
{
}

bool
SettlementOpFrame::doApply(Application& app, LedgerDelta& delta,
                           LedgerManager& ledgerManager)
{
    //  TODO placeholder
    return true;
}

bool
SettlementOpFrame::doCheckValid(Application& app)
{
    if (app.getLedgerManager().getCurrentLedgerVersion() < 2)
    {
        app.getMetrics()
            .NewMeter(
                {"op-set-options", "invalid", "invalid-data-old-protocol"},
                "operation")
            .Mark();
        innerResult().code(SETTLEMENT_NOT_SUPPORTED_YET);
        return false;
    }

    // TODO placeholder

    return true;
}
}
