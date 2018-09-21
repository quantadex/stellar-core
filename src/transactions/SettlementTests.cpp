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

    // create an account
    auto a1 = root.create("A", paymentAmount);

    const int64_t morePayment = paymentAmount / 2;

    int64_t trustLineLimit = INT64_MAX;

    int64_t trustLineStartingBalance = 20000;

    // sets up gateway account
    const int64_t gatewayPayment = minBalance2 + morePayment;
    auto gateway = root.create("gate", gatewayPayment);

    // sets up gateway2 account
    auto gateway2 = root.create("gate2", gatewayPayment);

    Asset idr = makeAsset(gateway, "IDR");
    Asset usd = makeAsset(gateway2, "USD");

    AccountFrame::pointer a1Account, rootAccount;
    rootAccount = loadAccount(root, *app);
    a1Account = loadAccount(a1, *app);
    REQUIRE(rootAccount->getMasterWeight() == 1);
    REQUIRE(rootAccount->getHighThreshold() == 0);
    REQUIRE(rootAccount->getLowThreshold() == 0);
    REQUIRE(rootAccount->getMediumThreshold() == 0);
    REQUIRE(a1Account->getBalance() == paymentAmount);
    REQUIRE(a1Account->getMasterWeight() == 1);
    REQUIRE(a1Account->getHighThreshold() == 0);
    REQUIRE(a1Account->getLowThreshold() == 0);
    REQUIRE(a1Account->getMediumThreshold() == 0);
    // root did 2 transactions at this point
    REQUIRE(rootAccount->getBalance() == (1000000000000000000 - paymentAmount -
                                          gatewayPayment * 2 - txfee * 3));
}
