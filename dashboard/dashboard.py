#!/usr/bin/env python3
"""
ExchangeCore Market Depth Dashboard
Visualizes reconstructed order book depth, trade executions, and volume from JSONL event logs.
"""

import argparse
import json
import os
import sys
from collections import defaultdict
from typing import Dict, List, Tuple

if "MPLCONFIGDIR" not in os.environ:
    os.environ["MPLCONFIGDIR"] = "/tmp/matplotlib_cache"

_CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
_PROJECT_ROOT = os.path.abspath(os.path.join(_CURRENT_DIR, ".."))
_PY_VER = f"python{sys.version_info.major}.{sys.version_info.minor}"
_VENV_SITE = os.path.join(_PROJECT_ROOT, ".venv", "lib", _PY_VER, "site-packages")
if os.path.exists(_VENV_SITE) and _VENV_SITE not in sys.path:
    sys.path.insert(0, _VENV_SITE)

HAS_MATPLOTLIB = True
try:
    import matplotlib
    import matplotlib.pyplot as plt
    import matplotlib.ticker as ticker
    import numpy as np
except ImportError:
    HAS_MATPLOTLIB = False

class OrderBookReconstructor:
    def __init__(self, target_symbol: str = "AAPL"):
        self.target_symbol = target_symbol

        self.bids: Dict[int, int] = defaultdict(int)
        self.asks: Dict[int, int] = defaultdict(int)

        self.live_orders: Dict[int, Tuple[str, int, int]] = {}

        self.trades: List[Dict] = []
        self.total_orders = 0
        self.total_cancelled = 0

    def process_event_line(self, raw_line: str):
        line = raw_line.strip()
        if not line or not line.startswith("{"):
            return

        try:
            ev = json.loads(line)
        except Exception:
            return

        ev_type = ev.get("event")
        sym = ev.get("symbol", "")
        if sym and sym != self.target_symbol:
            return

        if ev_type == "OrderAccepted":
            self.total_orders += 1
            order_id = ev.get("order_id")
            side = ev.get("side")
            otype = ev.get("type")
            price = ev.get("price", 0)
            qty = ev.get("quantity", 0)

            if otype == "Limit" and qty > 0:
                self.live_orders[order_id] = (side, price, qty)
                if side == "Buy":
                    self.bids[price] += qty
                else:
                    self.asks[price] += qty

        elif ev_type == "TradeExecuted":
            price = ev.get("price", 0)
            qty = ev.get("quantity", 0)
            ts = ev.get("timestamp", 0)
            buy_id = ev.get("buy_id")
            sell_id = ev.get("sell_id")

            self.trades.append({
                "timestamp": ts,
                "price": price / 100.0,
                "quantity": qty,
                "value": (price * qty) / 100.0
            })

            if buy_id in self.live_orders:
                side, p, rem = self.live_orders[buy_id]
                new_rem = max(0, rem - qty)
                self.bids[p] = max(0, self.bids[p] - min(rem, qty))
                if self.bids[p] == 0:
                    del self.bids[p]
                if new_rem == 0:
                    del self.live_orders[buy_id]
                else:
                    self.live_orders[buy_id] = (side, p, new_rem)

            if sell_id in self.live_orders:
                side, p, rem = self.live_orders[sell_id]
                new_rem = max(0, rem - qty)
                self.asks[p] = max(0, self.asks[p] - min(rem, qty))
                if self.asks[p] == 0:
                    del self.asks[p]
                if new_rem == 0:
                    del self.live_orders[sell_id]
                else:
                    self.live_orders[sell_id] = (side, p, new_rem)

        elif ev_type == "OrderCancelled":
            self.total_cancelled += 1
            order_id = ev.get("order_id")
            if order_id in self.live_orders:
                side, price, rem = self.live_orders[order_id]
                if side == "Buy":
                    self.bids[price] = max(0, self.bids[price] - rem)
                    if self.bids[price] == 0:
                        del self.bids[price]
                else:
                    self.asks[price] = max(0, self.asks[price] - rem)
                    if self.asks[price] == 0:
                        del self.asks[price]
                del self.live_orders[order_id]

        elif ev_type == "OrderFilled":
            order_id = ev.get("order_id")
            if order_id in self.live_orders:
                side, price, rem = self.live_orders[order_id]
                if side == "Buy":
                    self.bids[price] = max(0, self.bids[price] - rem)
                    if self.bids[price] == 0:
                        del self.bids[price]
                else:
                    self.asks[price] = max(0, self.asks[price] - rem)
                    if self.asks[price] == 0:
                        del self.asks[price]
                del self.live_orders[order_id]

def render_ascii_dashboard(reconstructor: OrderBookReconstructor, max_levels: int):
    """Fallback terminal-based visualizer when running headless or without GUI libraries."""
    print("\n" + "=" * 65)
    print(f"      EXCHANGECORE TERMINAL DASHBOARD — {reconstructor.target_symbol}")
    print("=" * 65)

    bid_prices = sorted(reconstructor.bids.keys(), reverse=True)[:max_levels]
    ask_prices = sorted(reconstructor.asks.keys())[:max_levels]

    print("\n[ASKS - Sell Orders]")
    print(f"{'Price ($)':>12} | {'Quantity':>10} | {'Visual Depth'}")
    print("-" * 50)
    for p in reversed(ask_prices):
        qty = reconstructor.asks[p]
        bar = "█" * min(30, max(1, qty // 50))
        print(f"{p/100.0:>12.2f} | {qty:>10,d} | \033[91m{bar}\033[0m")

    best_bid = (max(reconstructor.bids.keys()) / 100.0) if reconstructor.bids else 0.0
    best_ask = (min(reconstructor.asks.keys()) / 100.0) if reconstructor.asks else 0.0
    spread = (best_ask - best_bid) if (best_bid and best_ask) else 0.0

    print("-" * 50)
    print(f"  --- Inside Spread: ${spread:.2f} ({int(round(spread * 100))} cents) ---")
    print("-" * 50)

    print("[BIDS - Buy Orders]")
    print(f"{'Price ($)':>12} | {'Quantity':>10} | {'Visual Depth'}")
    print("-" * 50)
    for p in bid_prices:
        qty = reconstructor.bids[p]
        bar = "█" * min(30, max(1, qty // 50))
        print(f"{p/100.0:>12.2f} | {qty:>10,d} | \033[92m{bar}\033[0m")

    total_trades = len(reconstructor.trades)
    total_shares = sum(t["quantity"] for t in reconstructor.trades) if total_trades > 0 else 0
    total_value = sum(t["value"] for t in reconstructor.trades) if total_trades > 0 else 0.0

    print("\n[SUMMARY METRICS]")
    print(f"  • Total Orders Accepted: {reconstructor.total_orders:,}")
    print(f"  • Total Cancellations:   {reconstructor.total_cancelled:,}")
    print(f"  • Total Trades Executed: {total_trades:,}")
    print(f"  • Total Shares Traded:   {total_shares:,}")
    print(f"  • Total Dollar Value:    ${total_value:,.2f}")
    print("=" * 65 + "\n")

def render_dashboard(reconstructor: OrderBookReconstructor, max_levels: int, save_path: str = None):
    if not HAS_MATPLOTLIB:
        print("\nNote: matplotlib not available in current environment; rendering rich terminal dashboard:\n")
        render_ascii_dashboard(reconstructor, max_levels)
        return

    if save_path or not os.environ.get("DISPLAY"):
        matplotlib.use("Agg")

    plt.style.use("seaborn-v0_8-darkgrid" if "seaborn-v0_8-darkgrid" in plt.style.available else "default")
    fig = plt.figure(figsize=(15, 10), dpi=120)
    fig.patch.set_facecolor("#0e1117")

    gs = fig.add_gridspec(2, 2, height_ratios=[1.2, 1.0], hspace=0.3, wspace=0.25)

    ax_depth = fig.add_subplot(gs[0, 0])
    ax_trades = fig.add_subplot(gs[0, 1])
    ax_volume = fig.add_subplot(gs[1, 0])
    ax_stats = fig.add_subplot(gs[1, 1])

    for ax in [ax_depth, ax_trades, ax_volume, ax_stats]:
        ax.set_facecolor("#161b22")
        ax.tick_params(colors="#c9d1d9", labelsize=9)
        for spine in ax.spines.values():
            spine.set_color("#30363d")

    bid_prices = sorted(reconstructor.bids.keys(), reverse=True)[:max_levels]
    ask_prices = sorted(reconstructor.asks.keys())[:max_levels]

    bid_y = [p / 100.0 for p in bid_prices]
    bid_x = [reconstructor.bids[p] for p in bid_prices]

    ask_y = [p / 100.0 for p in ask_prices]
    ask_x = [reconstructor.asks[p] for p in ask_prices]

    if bid_y:
        ax_depth.barh(bid_y, bid_x, color="#2ea043", alpha=0.85, height=0.3, label="Bids (Buy)")
    if ask_y:
        ax_depth.barh(ask_y, ask_x, color="#da3633", alpha=0.85, height=0.3, label="Asks (Sell)")

    ax_depth.set_title(f"Market Depth (Top {max_levels} Levels) — {reconstructor.target_symbol}", color="#f0f6fc", fontsize=12, fontweight="bold")
    ax_depth.set_xlabel("Quantity (Shares)", color="#c9d1d9")
    ax_depth.set_ylabel("Price ($)", color="#c9d1d9")
    ax_depth.legend(facecolor="#21262d", edgecolor="#30363d", labelcolor="#c9d1d9")

    if reconstructor.trades:
        trade_indices = list(range(1, len(reconstructor.trades) + 1))
        trade_prices = [t["price"] for t in reconstructor.trades]
        trade_sizes = [max(10, min(100, t["quantity"] / 5)) for t in reconstructor.trades]

        ax_trades.scatter(trade_indices, trade_prices, s=trade_sizes, c="#58a6ff", alpha=0.6, edgecolors="none")
        ax_trades.plot(trade_indices, trade_prices, color="#1f6feb", linewidth=0.8, alpha=0.5)

        ax_trades.set_title("Executed Trade Prices", color="#f0f6fc", fontsize=12, fontweight="bold")
        ax_trades.set_xlabel("Trade Sequence #", color="#c9d1d9")
        ax_trades.set_ylabel("Execution Price ($)", color="#c9d1d9")
        ax_trades.yaxis.set_major_formatter(ticker.FormatStrFormatter("$%.2f"))

    if reconstructor.trades:
        cum_shares = np.cumsum([t["quantity"] for t in reconstructor.trades])
        ax_volume.plot(range(1, len(cum_shares) + 1), cum_shares, color="#e3b341", linewidth=2.0)
        ax_volume.fill_between(range(1, len(cum_shares) + 1), cum_shares, color="#e3b341", alpha=0.15)
        ax_volume.set_title("Cumulative Traded Volume", color="#f0f6fc", fontsize=12, fontweight="bold")
        ax_volume.set_xlabel("Trade Sequence #", color="#c9d1d9")
        ax_volume.set_ylabel("Shares Traded", color="#c9d1d9")
        ax_volume.yaxis.set_major_formatter(ticker.FuncFormatter(lambda x, p: f"{int(x):,}"))

    ax_stats.axis("off")
    total_trades = len(reconstructor.trades)
    total_shares = sum(t["quantity"] for t in reconstructor.trades) if total_trades > 0 else 0
    total_value = sum(t["value"] for t in reconstructor.trades) if total_trades > 0 else 0.0
    avg_price = (total_value / total_shares) if total_shares > 0 else 0.0
    fill_rate = (total_trades / max(1, reconstructor.total_orders)) * 100.0

    best_bid = (max(reconstructor.bids.keys()) / 100.0) if reconstructor.bids else 0.0
    best_ask = (min(reconstructor.asks.keys()) / 100.0) if reconstructor.asks else 0.0
    spread = (best_ask - best_bid) if (best_bid and best_ask) else 0.0

    stats_text = (
        f"EXCHANGECORE SUMMARY METRICS\n"
        f"━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n"
        f"  • Symbol:                   {reconstructor.target_symbol}\n"
        f"  • Total Orders Accepted:    {reconstructor.total_orders:,}\n"
        f"  • Orders Cancelled:         {reconstructor.total_cancelled:,}\n"
        f"  • Total Trades Executed:    {total_trades:,}\n"
        f"  • Total Shares Traded:      {total_shares:,}\n"
        f"  • Total Dollar Volume:      ${total_value:,.2f}\n"
        f"  • Volume-Weighted Avg Price: ${avg_price:.2f}\n"
        f"  • Current Best Bid:         ${best_bid:.2f}\n"
        f"  • Current Best Ask:         ${best_ask:.2f}\n"
        f"  • Inside Spread:            ${spread:.2f} ({int(round(spread * 100))} cents)\n"
        f"  • Trade Fill Ratio:         {fill_rate:.1f}%\n"
    )

    ax_stats.text(
        0.05, 0.90, stats_text,
        transform=ax_stats.transAxes,
        fontsize=11,
        fontfamily="monospace",
        color="#f0f6fc",
        verticalalignment="top",
        bbox=dict(boxstyle="round,pad=1.0", facecolor="#161b22", edgecolor="#30363d", alpha=0.9)
    )

    plt.suptitle("ExchangeCore — Real-Time Matching Engine Analytics & Depth Visualizer",
                 color="#f0f6fc", fontsize=15, fontweight="bold", y=0.98)

    if save_path:
        os.makedirs(os.path.dirname(os.path.abspath(save_path)), exist_ok=True)
        plt.savefig(save_path, facecolor=fig.get_facecolor(), edgecolor="none", bbox_inches="tight")
        print(f"Dashboard saved successfully to: {save_path}")
    else:
        plt.show()

def main():
    parser = argparse.ArgumentParser(description="ExchangeCore Market Depth Dashboard")
    parser.add_argument("--input", "-i", type=str, default="data/sample_events.jsonl",
                        help="Path to JSON Lines event log file generated by ExchangeCore")
    parser.add_argument("--save", "-s", type=str, default=None,
                        help="Optional file path to save output image (e.g. dashboard.png)")
    parser.add_argument("--symbol", type=str, default="AAPL",
                        help="Symbol to filter and visualize (default: AAPL)")
    parser.add_argument("--levels", type=int, default=15,
                        help="Number of book depth price levels to display (default: 15)")
    parser.add_argument("--terminal", action="store_true",
                        help="Force terminal ASCII visualization")

    args = parser.parse_args()

    if not os.path.exists(args.input):
        print(f"Error: Input file '{args.input}' not found.", file=sys.stderr)
        print("Please run ExchangeCore first to generate an event log:\n"
              "  ./build/exchange_main --replay data/sample_orders.jsonl data/sample_events.jsonl", file=sys.stderr)
        sys.exit(1)

    print(f"Loading and reconstructing events from {args.input}...")
    reconstructor = OrderBookReconstructor(target_symbol=args.symbol)

    with open(args.input, "r", encoding="utf-8") as f:
        for line in f:
            reconstructor.process_event_line(line)

    print(f"Reconstructed {reconstructor.total_orders} orders and {len(reconstructor.trades)} trades.")
    if args.terminal:
        render_ascii_dashboard(reconstructor, max_levels=args.levels)
    else:
        render_dashboard(reconstructor, max_levels=args.levels, save_path=args.save)

if __name__ == "__main__":
    main()
