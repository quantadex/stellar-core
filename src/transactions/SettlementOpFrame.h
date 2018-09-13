#pragma once

// Copyright 2015 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "transactions/OperationFrame.h"

namespace stellar
{
class SettlementOpFrame : public OperationFrame
{
    
    SettlementResult& innerResult() 
    {
        return mResult.tr().settlementResult();
    }
    
    SettlementOp const& mSettlement;

public:
    SettlementOpFrame(Operation const& op, OperationResult& res,
                      TransactionFrame& parentTx);

    bool doApply(Application& app, LedgerDelta& delta,
                 LedgerManager& ledgerManager) override;
    bool doCheckValid(Application& app) override;

    static SettlementResultCode
    getInnerCode(OperationResult const& res)
    {
        return res.tr().settlementResult().code();
    }
};
}
