// Copyright 2016 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "crypto/SecretKey.h"
#include "lib/catch.hpp"
#include "main/Config.h"
#include "test/test.h"

using namespace stellar;

namespace
{

bool
keyMatches(PublicKey& key, const std::vector<std::string>& keys)
{
    auto keyStr = KeyUtils::toStrKey<PublicKey>(key);
    return std::any_of(std::begin(keys), std::end(keys),
                       [&](const std::string& x) { return keyStr == x; });
}
}

TEST_CASE("resolve node id", "[config]")
{
    auto cfg = getTestConfig(0);
    auto validator1Key =
         std::string{"QAYR3BKOWKO7ITYDLBWNERKMW56MLCQYGMOIT6I2F744W4VH2TAUQZ4P"};
    auto validator2Key =
         std::string{"QB72TLJLZ4VUGTCJAWRGP7RZ47DPT6OPWYCFB6MF3NGSJINBAO4ZNDSA"};
    auto validator3Key =
         std::string{"QDN5G627L645Y4URHB32TAFTK5NUFZXI53YYRI4WFGPEAMWADMF3AEAN"};


    cfg.VALIDATOR_NAMES.emplace(std::make_pair(validator1Key, "core-testnet1"));
    cfg.VALIDATOR_NAMES.emplace(std::make_pair(validator2Key, "core-testnet2"));
    cfg.VALIDATOR_NAMES.emplace(std::make_pair(validator3Key, "core-testnet3"));

    SECTION("empty node id")
    {
        auto publicKey = PublicKey{};
        REQUIRE(!cfg.resolveNodeID("", publicKey));
    }

    SECTION("@")
    {
        auto publicKey = PublicKey{};
        REQUIRE(!cfg.resolveNodeID("@", publicKey));
    }

    SECTION("$")
    {
        auto publicKey = PublicKey{};
        REQUIRE(!cfg.resolveNodeID("@", publicKey));
    }

    SECTION("unique uppercase abbrevated id")
    {
        auto publicKey = PublicKey{};
        auto result = cfg.resolveNodeID("@QA", publicKey);
        REQUIRE(result);
        REQUIRE(keyMatches(publicKey, {validator1Key}));
    }

    SECTION("unique lowercase abbrevated id")
    {
        auto publicKey = PublicKey{};
        auto result = cfg.resolveNodeID("@qa", publicKey);
        REQUIRE(!result);
    }

    SECTION("non unique uppercase abbrevated id")
    {
        auto publicKey = PublicKey{};
        auto result = cfg.resolveNodeID("@QB", publicKey);
        REQUIRE(result);
        REQUIRE(keyMatches(publicKey, {validator2Key, validator3Key}));
    }

    SECTION("valid node alias")
    {
        auto publicKey = PublicKey{};
        auto result = cfg.resolveNodeID("$core-testnet1", publicKey);
        REQUIRE(result);
        REQUIRE(keyMatches(publicKey, {validator1Key}));
    }

    SECTION("uppercase node alias")
    {
        auto publicKey = PublicKey{};
        auto result = cfg.resolveNodeID("$CORE-TESTNET1", publicKey);
        REQUIRE(!result);
    }

    SECTION("node alias abbrevation")
    {
        auto publicKey = PublicKey{};
        auto result = cfg.resolveNodeID("$core", publicKey);
        REQUIRE(!result);
    }

    SECTION("existing full node id")
    {
        auto publicKey = PublicKey{};
        auto result = cfg.resolveNodeID(
            "QAYR3BKOWKO7ITYDLBWNERKMW56MLCQYGMOIT6I2F744W4VH2TAUQZ4P",
            publicKey);
        REQUIRE(result);
        REQUIRE(keyMatches(publicKey, {validator1Key}));
    }

    SECTION("abbrevated node id without prefix")
    {
        auto publicKey = PublicKey{};
        REQUIRE(!cfg.resolveNodeID("QAYR3BKOWKO7ITYDLBWNERKMW56MLCQYGMOIT6I2F744W4VH2", publicKey));
    }

    SECTION("existing lowercase full node id")
    {
        auto publicKey = PublicKey{};
        REQUIRE(!cfg.resolveNodeID(
            "qayr3bkowko7itydlbwnerkmw56mlcqygmoit6i2f744w4vh2",
            publicKey));
    }

    SECTION("non existing full node id")
    {
        auto publicKey = PublicKey{};
        auto result = cfg.resolveNodeID(
            "QD4VGM3PPWHULRA5PZQ2OLZJKDWL7PGZY6AWLCBWVSWIU6AFSXHTWK3L",
            publicKey);
        REQUIRE(result); // valid public key, but not in the config
        REQUIRE(!keyMatches(publicKey, {validator1Key, validator2Key, validator3Key}));
        // REQUIRE(!cfg.resolveNodeID(
        //     "QD4VGM3PPWHULRA5PZQ2OLZJKDWL7PGZY6AWLCBWVSWIU6AFSXHTWK3L",
        //     publicKey));
    }

    SECTION("invalid key type")
    {
        auto publicKey = PublicKey{};
        REQUIRE(!cfg.resolveNodeID(
            "TDTTOKJOEJXDBLATFZNTQRVA5MSCECMPOPC7CCCGL6AE5DKA7YCBJYJQ",
            publicKey));
    }
}

TEST_CASE("load example configs", "[config]")
{
    Config c;
    std::vector<std::string> testFiles = {"stellar-core_example.cfg",
                                          "stellar-core_standalone.cfg",
                                          "stellar-core_testnet.cfg"};
    for (auto const& fn : testFiles)
    {
        std::string fnPath = "./testdata/"; 
        fnPath += fn;
        SECTION("load config " + fnPath)
        {
            c.load(fnPath);
        }
    }
}
