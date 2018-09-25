// Copyright 2014 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0
#include "database/Database.h"
#include "ledger/LedgerManager.h"
#include "lib/catch.hpp"
#include "main/Application.h"
#include "main/Config.h"
#include "test/TestAccount.h"
#include "test/TestExceptions.h"
#include "test/TestMarket.h"
#include "test/TestUtils.h"
#include "test/TxTests.h"
#include "test/TestUtils.h"
#include "test/test.h"
#include "transactions/ChangeTrustOpFrame.h"
#include "transactions/MergeOpFrame.h"
#include "transactions/SettlementOpFrame.h"
#include "util/Logging.h"
#include "util/Timer.h"
#include "util/make_unique.h"

using namespace stellar;
using namespace stellar::txtest;

TEST_CASE("settlement", "[tx][settlement]")
{
    Config const& cfg = getTestConfig();

    VirtualClock clock;
    auto app = createTestApplication(clock, cfg);
    app->start();

    // set up world
    auto root = TestAccount::createRoot(*app);

    Asset xlm;

    int64_t txfee = app->getLedgerManager().getTxFee();

    // minimum balance necessary to hold 20 trust lines
    // numbers 20 and 100 taken out of thin air
    const int64_t minBalance2 =
        app->getLedgerManager().getMinBalance(20) + 100 * txfee;

    // minimum balance necessary to hold 20 trust lines and an offer
    const int64_t minBalance3 =
        app->getLedgerManager().getMinBalance(21) + 100 * txfee;

    const int64_t paymentAmount = minBalance2;

    // create a special settlement account with larger balance (it'll
    // be used to pay fees for all settlements)
    auto settler = root.create("settler", minBalance3 * 1000);

    const int64_t morePayment = paymentAmount / 2;

    int64_t trustLineLimit = INT64_MAX;

    int64_t trustLineStartingBalance = 20000;

    // sets up gateway account - issuers of the assets
    const int64_t gatewayPayment = minBalance2 + morePayment;
    auto gateway = root.create("gate", gatewayPayment);

    // sets up gateway2 account
    auto gateway2 = root.create("gate2", gatewayPayment);
    // issue  assets
    Asset idr = makeAsset(gateway, "IDR");
    Asset usd = makeAsset(gateway2, "USD");
    AccountFrame::pointer rootAccount, settlerAccount;
    rootAccount = loadAccount(root, *app);
    settlerAccount = loadAccount(settler, *app);
    
    REQUIRE(rootAccount->getMasterWeight() == 1);
    REQUIRE(rootAccount->getHighThreshold() == 0);
    REQUIRE(rootAccount->getLowThreshold() == 0);
    REQUIRE(rootAccount->getMediumThreshold() == 0);
    REQUIRE(settlerAccount->getMasterWeight() == 1);
    REQUIRE(settlerAccount->getHighThreshold() == 0);
    REQUIRE(settlerAccount->getLowThreshold() == 0);
    REQUIRE(settlerAccount->getMediumThreshold() == 0);

    // root did 3 transactions at this point
    REQUIRE( (1000000000000000000 -
              gatewayPayment * 2 - txfee * 3 - minBalance3*1000) == rootAccount->getBalance());

SECTION("prepareSimpleSettlement")
{
    auto market = TestMarket{*app};
    // create buyer and seller accounts
    auto b1 = root.create("buyer", paymentAmount);
    auto s1 = root.create("seller", paymentAmount);

    // unlimited trust for both assets to both accts
    b1.changeTrust(idr, trustLineLimit);
    b1.changeTrust(usd, trustLineLimit);
    s1.changeTrust(idr, trustLineLimit);
    s1.changeTrust(usd, trustLineLimit);

    // pre-settlement account status
    gateway.pay(b1, idr, trustLineStartingBalance * 5);
    gateway.pay(s1, idr, trustLineStartingBalance * 2);
    gateway2.pay(b1, usd, trustLineStartingBalance * 7); //140000
    gateway2.pay(s1, usd, trustLineStartingBalance * 3);

    auto b1Tli = loadTrustLine(b1, idr, *app);
    auto b1Tlu = loadTrustLine(b1, usd, *app);
    auto s1Tli = loadTrustLine(s1, idr, *app);
    auto s1Tlu = loadTrustLine(s1, usd, *app);

    market.requireBalances(
        { {b1, {{xlm, minBalance2 - txfee * 2}, {idr,trustLineStartingBalance * 5},
                                                {usd, trustLineStartingBalance * 7}}},
            {s1, {{xlm, minBalance2 - txfee * 2}, {idr,trustLineStartingBalance * 2},
                                                  {usd, trustLineStartingBalance * 3}} }
        });

    Operation sop(settlement(b1.getPublicKey(), s1.getPublicKey(),
                             25000, 15000, idr, usd));
    // create a TransactionFrame from the sop Operation, and return a
    // shared_ptr to it. The 'settler' account is the master for the
    // settlement transactions
    auto transf = settler.tx({sop}, 0);
    // apply the operation.
    applyTx(transf, *app);
    SettlementResult res = getFirstResult(*transf).tr().settlementResult();
    size_t fails, successes;
    fails = successes = 0;
    for (auto rescode : res.codesVec()) {
        if (rescode == SETTLEMENT_SUCCESS)
            ++successes;
        else
            ++fails;
    }

    // Reload the changed TrustLines. Duh...
    b1Tli = loadTrustLine(b1, idr, *app);
    b1Tlu = loadTrustLine(b1, usd, *app);
    s1Tli = loadTrustLine(s1, idr, *app);
    s1Tlu = loadTrustLine(s1, usd, *app);
    
    REQUIRE(s1Tli->getBalance() == trustLineStartingBalance * 2 - 25000);
    REQUIRE(b1Tli->getBalance() == trustLineStartingBalance * 5 + 25000 ); 
    REQUIRE(b1Tlu->getBalance() == trustLineStartingBalance * 7 - 15000);  
    REQUIRE(s1Tlu->getBalance() == trustLineStartingBalance * 3 + 15000);
    REQUIRE(successes == 1);
    REQUIRE(fails == 0);
}
}
