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

SettlementResultCode
SettlementOpFrame::validateTrustlines(AccountID const& accountId,
                                     Asset const& buyAss, Asset const& sellAss,
                                     medida::MetricsRegistry& metrics, 
                                     Database& db, LedgerDelta& delta,
                                     TrustFrame::pointer &trustLineBuyAsset,
                                     TrustFrame::pointer &trustLineSellAsset)
{
    if(sellAss.type() != ASSET_TYPE_NATIVE)
    {
        auto tlI =
            TrustFrame::loadTrustLineIssuer(accountId, sellAss, db, delta);
        trustLineSellAsset = tlI.first;
        if(!tlI.second) {
            metrics
                .NewMeter({"op-settlement", "invalid", "sell-no-issuer"},
                          "operation")
                .Mark();
            return SETTLEMENT_SELL_NO_ISSUER;
        }
        if(!trustLineSellAsset) { // don't have what we are trying to sell
            metrics
                .NewMeter({"op-settlement", "invalid", "sell-no-trust"},
                          "operation")
                .Mark();
            return SETTLEMENT_SELL_NO_TRUST;
        }
        if(!trustLineSellAsset->isAuthorized()) { // not authorized to sell
            metrics
                .NewMeter({"op-settlement", "invalid", "sell-not-authorized"},
                          "operation")
                .Mark();
            return SETTLEMENT_SELL_NOT_AUTHORIZED;
        }
    }
    if(buyAss.type() != ASSET_TYPE_NATIVE)
    {
        auto tlI =
            TrustFrame::loadTrustLineIssuer(accountId, buyAss, db, delta);
        trustLineBuyAsset = tlI.first;
        if(!tlI.second) {
            metrics
                .NewMeter({"op-settlement", "invalid", "buy-no-issuer"},
                          "operation")
                .Mark();
            return SETTLEMENT_BUY_NO_ISSUER;
        }
        if(!trustLineBuyAsset) { // the buyer can't hold what he's trying to buy
            metrics
                .NewMeter({"op-settlement", "invalid", "buy-no-trust"},
                          "operation")
                .Mark();
            return SETTLEMENT_BUY_NO_TRUST;
        }
        if(!trustLineBuyAsset->isAuthorized()) {
            // buyer not authorized to hold what he tries to buy 
            metrics
                .NewMeter({"op-settlement", "invalid", "buy-not-authorized"},
                          "operation")
                .Mark();
            return SETTLEMENT_BUY_NOT_AUTHORIZED;
        }
    }
    return SETTLEMENT_SUCCESS;
}
// the SettlementOpFrame.doCheckValid() and doApply() evaluate and
// attempt to apply every individual matchedOrder; they form a vector
// of results and return true; the processing of individual results
// happens on the application layer
// The doApply() works only on the matchedOrders, which have been
// validated by doCheckValid
bool
SettlementOpFrame::doApply(Application& app, LedgerDelta& delta,
                           LedgerManager& ledgerManager)
{
    Database& db = ledgerManager.getDatabase();
    medida::MetricsRegistry& metrics = app.getMetrics();

    soci::transaction sqlTx(db.getSession());

    LedgerDelta tempDelta(delta);
    // processing every MatchedOrder
    size_t ind = 0;
    
    for (auto matchedOrder : mSettlement.matchedOrders) {
        if(innerResult().codesVec()[ind] != SETTLEMENT_SUCCESS){
            ++ind;
            continue; // this one is invalid
        }
        else
        {   
            Asset const& bass = matchedOrder.assetBuy;
            AccountID const& bacc = matchedOrder.buyer;
            int64_t const bamt = matchedOrder.amountBuy;
            Asset const& sass = matchedOrder.assetSell;
            AccountID const& sacc = matchedOrder.seller;
            int64_t const samt = matchedOrder.amountSell;

            // make sure both buyer and seller accts exist, and initialize
            // member data shared_ptrs. Moved from doCheckValid to
            // avoid an extra trip to DB for every accountID 
            mAccBuyer = AccountFrame::loadAccount(bacc, app.getDatabase());
            if(!mAccBuyer) {
                // record the failure in the result vector and continue
                app.getMetrics()
                    .NewMeter(
                        {"op-settlement", "invalid", "buyer-account-invalid"},
                        "operation")
                    .Mark();
                innerResult().codesVec()[ind++] = SETTLEMENT_BUYER_ACCOUNT_INVALID;
                continue;
            }  
            mAccSeller = AccountFrame::loadAccount(sacc, app.getDatabase());
            if(!mAccSeller) {
                // record the failure in the result vector and continue
                app.getMetrics()
                    .NewMeter(
                        {"op-settlement", "invalid", "seller-account-invalid"},
                        "operation")
                    .Mark();
                innerResult().codesVec()[ind++] = SETTLEMENT_SELLER_ACCOUNT_INVALID;
                continue;
            }
            // do trustline checks for the validated settlement, and
            // apply the order 
            //
            SettlementResultCode validationRes;
            if((validationRes = validateTrustlines(bacc, bass, sass, metrics, db,
                                                 delta, mBuyAssetBuyer,
                                                  mSellAssetBuyer)) != SETTLEMENT_SUCCESS) {
                innerResult().codesVec()[ind++] = validationRes;
                continue; 
            }
            if((validationRes = validateTrustlines(sacc, bass, sass, metrics, db,
                                                 delta, mBuyAssetSeller,
                                                  mSellAssetSeller)) != SETTLEMENT_SUCCESS) {
                innerResult().codesVec()[ind++] = validationRes;
                continue; 
            }
            //record validation success and go on with this matchedOrder
            innerResult().codesVec()[ind] = validationRes; 
            //trustlines validated - do the settlement
            //
            // max amt of buy asset buyer can buy, and max amt of sell
            // asset buyer can sell 
            int64_t maxBAssCanBuy, maxSAssCanSell; 
            int64_t maxSAssBHas = mSellAssetBuyer->getBalance();
            
            if(bass.type() == ASSET_TYPE_NATIVE)
            {
                maxBAssCanBuy = INT64_MAX;
            }
            else
            {
                maxBAssCanBuy = mBuyAssetBuyer->getMaxAmountReceive();
                if (maxBAssCanBuy < bamt) {
                    metrics
                        .NewMeter({"op-settlement", "invalid", "line-full"},
                          "operation")
                        .Mark();
                    innerResult().codesVec()[ind++] = SETTLEMENT_LINE_FULL;
                    continue;
                }
            }
            // TODO check correctness here
            if (sass.type() == ASSET_TYPE_NATIVE)
            {
                maxSAssCanSell = INT64_MAX;
            }
            else
            {
                maxSAssCanSell = mSellAssetSeller->getMaxAmountReceive();
            }
            if(maxSAssCanSell < maxSAssBHas)
                maxSAssCanSell = maxSAssBHas;
            if(maxSAssCanSell < samt) {
                metrics
                    .NewMeter({"op-settlement", "invalid", "line-full"},
                              "operation")
                    .Mark();
                innerResult().codesVec()[ind++] = SETTLEMENT_SELLER_LINE_FULL;
                continue;
            }
            // Buyer acc: add the buy asset, and deduct the sell asset
            if (bass.type() == ASSET_TYPE_NATIVE)
            {
                if (!mAccBuyer->addBalance(bamt))
                {
                    // this would indicate a bug in OfferExchange
                    // TODO should we continue with other
                    // matchedOrders from the settlement?
                    // I think - yes;  we'll throw during the final
                    // pass, if required
                    innerResult().codesVec()[ind++] = SETTLEMENT_BUY_OVER_LIMIT;
                    continue;
                }
                mAccBuyer->storeChange(tempDelta, db);
            }
            else
            {
                if(!mBuyAssetBuyer->addBalance(bamt))
                {
                    innerResult().codesVec()[ind++] = SETTLEMENT_BUY_OVER_LIMIT;
                    continue;
                }
                mBuyAssetBuyer->storeChange(tempDelta, db);
            }
            if(sass.type() == ASSET_TYPE_NATIVE)
            {
                if (!mAccBuyer->addBalance(-samt))
                {
                    innerResult().codesVec()[ind++] = SETTLEMENT_SELL_OVER_BALANCE;
                    continue;
                }
                mAccBuyer->storeChange(tempDelta, db);
            }
            else
            {
                if(!mSellAssetBuyer->addBalance(-samt))
                {
                    innerResult().codesVec()[ind++] = SETTLEMENT_SELL_OVER_BALANCE;
                    continue;
                }
                mSellAssetBuyer->storeChange(tempDelta, db);
            }

            // Seller acct - add the sell asset, and deduct the buy
            // asset
            if (bass.type() == ASSET_TYPE_NATIVE)
            {
                if (!mAccSeller->addBalance(-bamt))
                {
                    innerResult().codesVec()[ind++] = SETTLEMENT_BUY_OVER_LIMIT;
                    continue;
                }
                mAccSeller->storeChange(tempDelta, db);
            }
            else
            {
                if(!mBuyAssetSeller->addBalance(-bamt))
                {
                    innerResult().codesVec()[ind++] = SETTLEMENT_BUY_OVER_LIMIT;
                    continue;
                }
                mBuyAssetSeller->storeChange(tempDelta, db);
            }
            if(sass.type() == ASSET_TYPE_NATIVE)
            {
                if (!mAccSeller->addBalance(samt))
                {
                    innerResult().codesVec()[ind++] = SETTLEMENT_SELL_OVER_BALANCE;
                    continue;
                }
                mAccSeller->storeChange(tempDelta, db);
            }
            else
            {
                if(!mSellAssetSeller->addBalance(samt))
                {
                    innerResult().codesVec()[ind++] = SETTLEMENT_SELL_OVER_BALANCE;
                    continue;
                }
                mSellAssetSeller->storeChange(tempDelta, db);
            }
        }
        ++ind;
    } // matchedOrders loop

    sqlTx.commit();
    tempDelta.commit();
    app.getMetrics()
        .NewMeter({"op-settlement", "success", "apply"}, "operation")
        .Mark();
    //TODO? additional error reporting for non-successful
    //matchedOrders (i.e. elements of
    //the innerResult().codesVec())
    return true;
}

bool
SettlementOpFrame::doCheckValid(Application& app)
{
    // if (app.getLedgerManager().getCurrentLedgerVersion() < 2)
    // {
    //     app.getMetrics()
    //         .NewMeter(
    //             {"op-settlement", "invalid", "invalid-data-old-protocol"},
    //             "operation")
    //         .Mark();
    //     innerResult().codesVec()[0] = SETTLEMENT_NOT_SUPPORTED_YET;
    //     return false;
    // }
    xdr::xvector<stellar::SettlementResultCode>& vec = innerResult().codesVec();
    vec.resize(mSettlement.matchedOrders.size());
    // before processing individual MatchedOrders, make sure that the
    // source account for this OpFrame is the settlement account (by
    // ID from the Config)  
    AccountID srcAccID = getSourceID(); 
    if (KeyUtils::toStrKey(srcAccID) != app.getConfig().SETTLEMENT_ACC_ID) {
        app.getMetrics()
            .NewMeter(
                {"op-settlement", "invalid", "settlement-invalid-source-acct"},
                "operation")
            .Mark();
        innerResult().codesVec()[0] = SETTLEMENT_SOURCE_ACCOUNT_INVALID;
        return false;
    }

    // checking every MatchedOrder
    size_t ind = 0;
    for (auto matchedOrder : mSettlement.matchedOrders) {
        
        // check asset validity
        Asset const& sass = matchedOrder.assetSell;
        Asset const& bass = matchedOrder.assetBuy;
        if (!isAssetValid(sass) || !isAssetValid(bass)) {
            app.getMetrics()
                .NewMeter(
                    {"op-settlement", "invalid", "settlement-invalid-asset"},
                    "operation")
                .Mark();
            innerResult().codesVec()[ind++] = SETTLEMENT_INVALID_ASSET;
            continue;
        }
        if (matchedOrder.assetBuy == matchedOrder.assetSell) {
            app.getMetrics()
                .NewMeter(
                    {"op-settlement", "invalid", "settlement_assets_identical"},
                    "operation")
                .Mark();
            innerResult().codesVec()[ind++] = SETTLEMENT_ASSETS_IDENTICAL;
            continue;
        }
        if (matchedOrder.amountBuy < 0 || matchedOrder.amountSell < 0) {
            app.getMetrics()
                .NewMeter(
                    {"op-settlement", "invalid", "settlement-negative-amount"},
                    "operation")
                .Mark();
            innerResult().codesVec()[ind++] = SETTLEMENT_NEGATIVE_AMOUNT;
            continue;
        }
        if (matchedOrder.buyer == matchedOrder.seller) {
            app.getMetrics()
                .NewMeter(
                    {"op-settlement", "invalid", "settlement_cross_self"},
                    "operation")
                .Mark();
            innerResult().codesVec()[ind++] = SETTLEMENT_CROSS_SELF;
            continue;
        }
        innerResult().codesVec()[ind++] = SETTLEMENT_SUCCESS;
    }
    return true;
}
}
