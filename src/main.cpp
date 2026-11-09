#include "exchange/types.hpp"
#include "exchange/order_manager.hpp"
#include "exchange/matching_engine.hpp"
#include "exchange/execution_reporter.hpp"
#include "exchange/event_log.hpp"
#include "exchange/market_data_generator.hpp"
#include "exchange/replay_engine.hpp"

#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>

namespace {
void printBanner() {
    std::cout << "========================================================\n"
              << "       ExchangeCore — C++20 Stock Matching Engine        \n"
              << "========================================================\n";
}

void printUsage(const char* prog) {
    std::cout << "Usage:\n"
              << "  " << prog << " --demo\n"
              << "      Runs an educational walkthrough of order book matching.\n\n"
              << "  " << prog << " --generate <output_file> [event_count=10000] [seed=42]\n"
              << "      Generates synthetic order stream to a JSON Lines file.\n\n"
              << "  " << prog << " --replay <input_file> [events_out.jsonl]\n"
              << "      Replays historical order stream and optionally logs all engine events.\n\n"
              << "  " << prog << " --help\n"
              << "      Shows this help menu.\n";
}

void printBookDepth(const exchange::OrderBook& book) {
    std::cout << "\n--- Order Book: " << book.symbol() << " ---\n";
    auto asks = book.getAskDepth(5);
    auto bids = book.getBidDepth(5);

    std::cout << "  [ASKS - Sell Side]\n";
    if (asks.empty()) {
        std::cout << "    (Empty)\n";
    } else {
        for (auto it = asks.rbegin(); it != asks.rend(); ++it) {
            std::cout << "    $" << std::fixed << std::setprecision(2)
                      << (it->price / 100.0) << " | Qty: " << it->total_quantity
                      << " (" << it->order_count << " orders)\n";
        }
    }

    if (auto sp = book.spread(); sp.has_value()) {
        std::cout << "  --- Spread: $" << std::fixed << std::setprecision(2)
                  << (*sp / 100.0) << " (" << *sp << " cents) ---\n";
    } else {
        std::cout << "  --- Spread: N/A ---\n";
    }

    std::cout << "  [BIDS - Buy Side]\n";
    if (bids.empty()) {
        std::cout << "    (Empty)\n";
    } else {
        for (const auto& b : bids) {
            std::cout << "    $" << std::fixed << std::setprecision(2)
                      << (b.price / 100.0) << " | Qty: " << b.total_quantity
                      << " (" << b.order_count << " orders)\n";
        }
    }
    std::cout << "-------------------------\n";
}

void runDemo() {
    std::cout << "\n[DEMO] Starting educational matching scenario on symbol AAPL...\n";

    exchange::OrderManager order_mgr;
    exchange::ExecutionReporter reporter;
    exchange::MatchingEngine engine(order_mgr, reporter);

    reporter.onTrade([](const exchange::Trade& t) {
        std::cout << "  [TRADE] Buy #" << t.buy_order_id << " matched Sell #" << t.sell_order_id
                  << " | " << t.quantity << " shares @ $" << std::fixed << std::setprecision(2)
                  << (t.price / 100.0) << "\n";
    });

    reporter.onOrderCancelled([](exchange::OrderId id, std::string_view sym, exchange::Quantity rem) {
        std::cout << "  [CANCEL] Order #" << id << " on " << sym << " cancelled (" << rem << " shares remaining)\n";
    });

    std::cout << "\n1. Adding resting Sell Limit orders:\n";
    std::cout << "   - Order #1: Sell 100 @ $151.00\n";
    std::cout << "   - Order #2: Sell 200 @ $152.00\n";
    std::cout << "   - Order #3: Sell 150 @ $151.00 (time priority behind #1)\n";

    auto* s1 = order_mgr.createOrder(exchange::Side::Sell, exchange::OrderType::Limit, "AAPL", 15100, 100);
    engine.processOrder(s1);
    auto* s2 = order_mgr.createOrder(exchange::Side::Sell, exchange::OrderType::Limit, "AAPL", 15200, 200);
    engine.processOrder(s2);
    auto* s3 = order_mgr.createOrder(exchange::Side::Sell, exchange::OrderType::Limit, "AAPL", 15100, 150);
    engine.processOrder(s3);

    std::cout << "\n2. Adding resting Buy Limit orders:\n";
    std::cout << "   - Order #4: Buy 300 @ $149.00\n";
    std::cout << "   - Order #5: Buy 100 @ $148.50\n";

    auto* b1 = order_mgr.createOrder(exchange::Side::Buy, exchange::OrderType::Limit, "AAPL", 14900, 300);
    engine.processOrder(b1);
    auto* b2 = order_mgr.createOrder(exchange::Side::Buy, exchange::OrderType::Limit, "AAPL", 14850, 100);
    engine.processOrder(b2);

    printBookDepth(*engine.getBook("AAPL"));

    std::cout << "\n3. Submitting an aggressive crossing Buy Limit order:\n";
    std::cout << "   - Order #6: Buy 180 @ $151.50\n";
    std::cout << "   Expected:\n";
    std::cout << "     - 100 shares fill at $151.00 against #1 (first in queue)\n";
    std::cout << "     - 80 shares fill at $151.00 against #3 (second in queue)\n";
    std::cout << "     - Execution price is $151.00 (price improvement from $151.50!)\n";

    auto* b3 = order_mgr.createOrder(exchange::Side::Buy, exchange::OrderType::Limit, "AAPL", 15150, 180);
    engine.processOrder(b3);

    printBookDepth(*engine.getBook("AAPL"));

    std::cout << "\n4. Cancelling resting order #2 (Sell 200 @ $152.00):\n";
    bool cancelled = engine.cancelOrder(s2->id);
    std::cout << "   Cancel result: " << (cancelled ? "SUCCESS" : "FAILED") << "\n";

    printBookDepth(*engine.getBook("AAPL"));

    std::cout << "\n5. Submitting Market Buy Order for 100 shares:\n";
    std::cout << "   - Order #7: Market Buy 100 shares\n";
    std::cout << "   Expected: matches remaining 70 shares of #3 @ $151.00; remainder 30 shares cancelled due to no liquidity.\n";

    auto* m1 = order_mgr.createOrder(exchange::Side::Buy, exchange::OrderType::Market, "AAPL", 0, 100);
    engine.processOrder(m1);

    printBookDepth(*engine.getBook("AAPL"));

    std::cout << "\nDemo complete. Total trades recorded: " << reporter.totalTrades()
              << ", volume: " << reporter.totalSharesTraded() << " shares ($"
              << (reporter.totalVolumeValue() / 100.0) << ").\n";
}
}

int main(int argc, char* argv[]) {
    printBanner();

    if (argc < 2) {
        printUsage(argv[0]);
        return 0;
    }

    std::string_view mode = argv[1];

    if (mode == "--help" || mode == "-h") {
        printUsage(argv[0]);
        return 0;
    }

    if (mode == "--demo") {
        runDemo();
        return 0;
    }

    if (mode == "--generate") {
        if (argc < 3) {
            std::cerr << "Error: Output filepath required for --generate\n";
            return 1;
        }
        std::filesystem::path out_path = argv[2];
        std::size_t count = (argc >= 4) ? std::stoull(argv[3]) : 10000;
        std::uint64_t seed = (argc >= 5) ? std::stoull(argv[4]) : 42;

        exchange::GeneratorConfig config{
            .symbol = "AAPL",
            .base_price = 15000,
            .price_tick = 5,
            .price_std_dev = 100,
            .min_quantity = 10,
            .max_quantity = 500,
            .market_order_ratio = 0.10,
            .cancel_ratio = 0.15,
            .seed = seed,
            .total_events = count
        };

        std::cout << "Generating " << count << " synthetic events (seed=" << seed << ") to "
                  << out_path << "...\n";
        exchange::MarketDataGenerator gen(config);
        gen.generateToFile(out_path);
        std::cout << "Successfully generated market data.\n";
        return 0;
    }

    if (mode == "--replay") {
        if (argc < 3) {
            std::cerr << "Error: Input filepath required for --replay\n";
            return 1;
        }
        std::filesystem::path in_path = argv[2];
        std::unique_ptr<exchange::EventLog> event_log;
        if (argc >= 4) {
            event_log = std::make_unique<exchange::EventLog>(argv[3]);
            std::cout << "Engine event logging enabled -> " << argv[3] << "\n";
        }

        exchange::OrderManager order_mgr;
        exchange::ExecutionReporter reporter(event_log.get());
        exchange::MatchingEngine engine(order_mgr, reporter);
        exchange::ReplayEngine replay(order_mgr, engine, reporter);

        std::cout << "Replaying order stream from: " << in_path << "...\n";
        auto stats = replay.replayFromFile(in_path);

        std::cout << "\n--- Replay Summary ---\n"
                  << "Orders Submitted: " << stats.orders_submitted << "\n"
                  << "Orders Cancelled: " << stats.orders_cancelled << "\n"
                  << "Trades Executed:  " << stats.trades_executed << "\n"
                  << "Shares Traded:    " << stats.shares_traded << "\n"
                  << "Volume Traded:    $" << std::fixed << std::setprecision(2)
                  << (stats.volume_value_cents / 100.0) << "\n"
                  << "Elapsed Time:     " << (stats.elapsed_wall_time.count() / 1000.0) << " ms\n"
                  << "Throughput:       " << std::fixed << std::setprecision(0)
                  << stats.orders_per_second << " ops/sec\n"
                  << "-----------------------\n";
        return 0;
    }

    std::cerr << "Unknown option: " << mode << "\n";
    printUsage(argv[0]);
    return 1;
}
