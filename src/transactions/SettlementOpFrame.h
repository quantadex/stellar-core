#pragma once

// Copyright 2015 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "transactions/OperationFrame.h"
#include "ledger/TrustFrame.h"

namespace stellar
{
class SettlementOpFrame : public OperationFrame
{
    
    SettlementResult& innerResult() 
    {
        return mResult.tr().settlementResult();
    }
    
    SettlementOp const& mSettlement;

    // Trust frames for buyer and seller for buy and sell assets
    TrustFrame::pointer mSellAssetBuyer;
    TrustFrame::pointer mBuyAssetBuyer;

    TrustFrame::pointer mSellAssetSeller;
    TrustFrame::pointer mBuyAssetSeller;

    AccountFrame::pointer mAccBuyer;
    AccountFrame::pointer mAccSeller;

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

    SettlementResultCode validateTrustlines(AccountID const& accountId,
                                           Asset const& buyAss, Asset const& sellAss,
                                           medida::MetricsRegistry& metrics,
                                           Database& db, LedgerDelta& delta,
                                           TrustFrame::pointer &trustLineBuyAsset,
                                           TrustFrame::pointer &trustLineSellAsset);
};
}
