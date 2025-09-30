#include <gtest/gtest.h>
#include <iostream>
#include <sstream>

#include "Engine.h"

namespace {
template <typename Fn>
std::string captureStdout(Fn&& fn) {
    std::ostringstream buffer;
    auto* old = std::cout.rdbuf(buffer.rdbuf());
    fn();
    std::cout.rdbuf(old);
    return buffer.str();
}
} // namespace

TEST(EngineTest, BasicBuySellMatch) {
    Engine e;
    e.onNewOrder({"s1", 100, 5, Side::Sell, TIF::GFD});

    std::string trades = captureStdout([&]() {
        e.onNewOrder({"b1", 110, 3, Side::Buy, TIF::IOC});
    });

    EXPECT_NE(trades.find("TRADE s1 100 3 b1 110 3"), std::string::npos);

    std::string book = captureStdout([&]() { e.onPrint(); });
    EXPECT_NE(book.find("SELL:\n100 2\n"), std::string::npos);
}

TEST(EngineTest, PrintSellsAscending) {
    Engine e;
    e.onNewOrder({"s1", 105, 1, Side::Sell, TIF::GFD});
    e.onNewOrder({"s2", 95, 2, Side::Sell, TIF::GFD});

    std::string book = captureStdout([&]() { e.onPrint(); });
    auto sellPos = book.find("SELL:\n");
    ASSERT_NE(sellPos, std::string::npos);
    auto buyPos = book.find("BUY:\n");
    ASSERT_NE(buyPos, std::string::npos);
    std::string sells = book.substr(sellPos, buyPos - sellPos);
    EXPECT_NE(sells.find("SELL:\n95 2\n105 1\n"), std::string::npos);
}

TEST(EngineTest, ModifyUpdatesExistingOrder) {
    Engine e;
    e.onNewOrder({"o2", 200, 5, Side::Sell, TIF::GFD});

    e.onModify("o2", Side::Sell, 150, 3);

    std::string book = captureStdout([&]() { e.onPrint(); });
    EXPECT_NE(book.find("SELL:\n150 3\n"), std::string::npos);
    EXPECT_TRUE(e.hasId("o2"));
}

TEST(EngineTest, ModifyMovesOrderToOppositeBook) {
    Engine e;
    e.onNewOrder({"o3", 210, 4, Side::Sell, TIF::GFD});

    e.onModify("o3", Side::Buy, 220, 4);

    std::string book = captureStdout([&]() { e.onPrint(); });
    auto buyPos = book.find("BUY:\n");
    ASSERT_NE(buyPos, std::string::npos);
    EXPECT_EQ(book.substr(0, buyPos), std::string("SELL:\n"));
    EXPECT_NE(book.find("BUY:\n220 4\n"), std::string::npos);
}

TEST(EngineTest, ModifyInvalidIdNoop) {
    Engine e;
    e.onNewOrder({"o4", 99, 1, Side::Sell, TIF::GFD});

    e.onModify("missing", Side::Buy, 50, 2);

    EXPECT_TRUE(e.hasId("o4"));
}

TEST(EngineTest, ModifyToZeroQuantityCancels) {
    Engine e;
    e.onNewOrder({"o5", 120, 2, Side::Buy, TIF::GFD});

    e.onModify("o5", Side::Buy, 115, 0);

    EXPECT_FALSE(e.hasId("o5"));
}

TEST(EngineTest, ModifyToInvalidPriceCancels) {
    Engine e;
    e.onNewOrder({"o6", 130, 3, Side::Buy, TIF::GFD});

    e.onModify("o6", Side::Buy, -1, 3);

    EXPECT_FALSE(e.hasId("o6"));
}
