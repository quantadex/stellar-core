// Copyright 2016 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "lib/catch.hpp"
#include "main/Application.h"
#include "test/TestAccount.h"
#include "test/TestExceptions.h"
#include "test/TestUtils.h"
#include "test/TxTests.h"
#include "test/test.h"
#include "util/Timer.h"

using namespace stellar;
using namespace stellar::txtest;

TEST_CASE("assetformat", "[tx][assetformat]")
{
    auto const& cfg = getTestConfig();

    VirtualClock clock;
    auto app = createTestApplication(clock, cfg);

    app->start();

    // set up world
    auto root = TestAccount::createRoot(*app);

    const int64_t trustLineLimit = INT64_MAX;
    const int64_t trustLineStartingBalance = 20000;

    auto const minBalance20 = app->getLedgerManager().getMinBalance(20);

    auto gateway = root.create("gw", minBalance20);
    auto a1 = root.create("A1", minBalance20);
    auto a2 = root.create("A2", minBalance20);

    auto longone = makeLongerAsset(gateway, "veryveryverylongasset");
    
    SECTION("checktrustline")
    {
        // trust line auto created at payment
        gateway.pay(a1, longone, trustLineStartingBalance);
        auto a1Tla = loadTrustLine(a1, longone, *app);
        REQUIRE(a1Tla->getBalance() == trustLineStartingBalance);
    }
}
