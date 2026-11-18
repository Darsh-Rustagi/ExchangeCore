# ExchangeCore — Data Structures & Algorithms Deep Dive

This document provides a comprehensive technical analysis of every data structure, algorithmic trade-off, and computational complexity model utilized in ExchangeCore.

---

## 1. Summary Complexity Matrix

| Operation | Method | Time Complexity | Auxiliary Space | Explanation |
|---|---|---|---|---|
| **Best Price Lookup** | `bestBid()`, `bestAsk()` | $\mathcal{O}(1)$ | $\mathcal{O}(1)$ | Map's `begin()` points directly to root/leftmost/rightmost node |
| **Top-of-Book Peek** | `peekBestBid()`, `peekBestAsk()` | $\mathcal{O}(1)$ | $\mathcal{O}(1)$ | Dereference front pointer of doubly-linked list |
| **Limit Order Insertion** | `addOrder(Order*)` | $\mathcal{O}(\log P)$ | $\mathcal{O}(1)$ | $P$ = active price levels. Tree traversal + list `push_back` |
| **Order Cancellation** | `removeOrder(OrderId)` | $\mathcal{O}(1)$ amortized | $\mathcal{O}(1)$ | Hash lookup $\mathcal{O}(1)$ + list iterator splice/erase $\mathcal{O}(1)$ |
| **Match Single Order** | `processOrder(Order*)` | $\mathcal{O}(K)$ | $\mathcal{O}(1)$ | $K$ = number of matched orders at top-of-book |
| **Depth Snapshot** | `getBidDepth(N)` | $\mathcal{O}(N)$ | $\mathcal{O}(N)$ | In-order iterator scan through top $N$ price levels |

---

## 2. Order Book Design: The Hybrid Tree-List Pattern

```
BIDS (std::map<Price, std::list<Order*>, std::greater<Price>>)
┌─────────────────────────────────────────────────────────────┐
│ Key: $151.00  ──> [ Order #1 (100) ] <──> [ Order #3 (50) ] │  <-- Best Bid (begin())
├─────────────────────────────────────────────────────────────┤
│ Key: $150.50  ──> [ Order #4 (200) ]                        │
├─────────────────────────────────────────────────────────────┤
│ Key: $149.00  ──> [ Order #7 (80)  ] <──> [ Order #9 (120)] │
└─────────────────────────────────────────────────────────────┘

ASKS (std::map<Price, std::list<Order*>, std::less<Price>>)
┌─────────────────────────────────────────────────────────────┐
│ Key: $151.50  ──> [ Order #2 (150) ]                        │  <-- Best Ask (begin())
├─────────────────────────────────────────────────────────────┤
│ Key: $152.00  ──> [ Order #5 (100) ] <──> [ Order #6 (90) ] │
├─────────────────────────────────────────────────────────────┤
│ Key: $153.00  ──> [ Order #8 (300) ]                        │
└─────────────────────────────────────────────────────────────┘
```

### Why a Tree of Linked Lists?
A stock exchange order book has two competing ordering requirements:
1. **Price Priority**: Better prices execute before worse prices.
   - For buyers: higher price is better $\rightarrow$ strictly descending sort.
   - For sellers: lower price is better $\rightarrow$ strictly ascending sort.
2. **Time Priority**: At identical price levels, older orders execute before newer ones (First-In, First-Out queue).

### `std::map` (Self-Balancing Red-Black Tree)
- Maintains price levels in sorted order dynamically.
- `begin()` provides the top-of-book (best bid or best ask) in $\mathcal{O}(1)$ time.
- Adding a new price level takes $\mathcal{O}(\log P)$ time, where $P$ is the number of distinct price levels (typically between 50 and 5,000 in active equities).

### `std::list` (Doubly-Linked List)
- Serves as the FIFO queue for each price level.
- `push_back()` appends incoming orders in $\mathcal{O}(1)$ time.
- `pop_front()` removes filled orders in $\mathcal{O}(1)$ time.
- Crucially, `std::list` supports **iterator-based erasure in $\mathcal{O}(1)$ time**:
  ```cpp
  list.erase(stored_iterator); // O(1) pointer relinking without shifting elements
  ```

---

## 3. Constant-Time $\mathcal{O}(1)$ Cancellation Index

### The Naive Problem
If a client sends `cancelOrder(id = 8472)`, a naive order book would have to:
1. Search all price levels in the tree ($\mathcal{O}(P)$).
2. Linearly traverse the list at each price level ($\mathcal{O}(L)$).
3. Worst-case cost: $\mathcal{O}(P \times L)$ — catastrophic when processing thousands of cancellations per second.

### The ExchangeCore Solution
ExchangeCore uses a secondary hash table index:

```cpp
struct OrderLocation {
    Side side;
    Price price;
    std::list<Order*>::iterator iterator;
};

std::unordered_map<OrderId, OrderLocation> order_locations_;
```

When an order is inserted:
```cpp
auto& list = bids_[order->price];
list.push_back(order);
order_locations_[order->id] = OrderLocation{Side::Buy, order->price, std::prev(list.end())};
```

When an order is cancelled:
```cpp
auto loc = order_locations_.find(id);         // 1. O(1) hash lookup
bids_[loc->price].erase(loc->iterator);       // 2. O(1) doubly-linked list node unlinking
if (bids_[loc->price].empty()) {
    bids_.erase(loc->price);                  // 3. O(log P) clean up empty price level
}
order_locations_.erase(loc);                  // 4. O(1) hash table erase
```
Total cancellation time is **instantaneous and decoupled from the total volume of orders in the book**.

---

## 4. Integer Cent Pricing vs Floating-Point Pitfalls

In financial engineering, floating-point numbers (`float` or `double`) are dangerous:

```cpp
// FLOATING POINT DANGER:
double a = 0.10 + 0.20; // Evaluates to 0.3000000000000000444
double b = 0.30;
assert(a == b); // FAILS!
```

### IEEE 754 Roundoff Hazards
1. **Spread calculations**: A spread that should be \$0.00 might compute as \$0.00000000000000005, causing false spread crossings or stuck orders.
2. **Accounting discrepancies**: In millions of cumulative trades, fractional cents accumulate into balancing errors.

### Fixed-Point Integer Representation
ExchangeCore represents all prices as fixed 64-bit signed integers in **cents**:
```cpp
using Price = std::int64_t; // 15000 = $150.00
```
- **Arithmetic is exact**: Addition, subtraction, and comparisons never introduce rounding artifacts.
- **Microsecond comparisons**: Single-cycle integer comparison on modern 64-bit ALUs.
- **Range**: `int64_t` can represent up to $\pm 9.22 \times 10^{18}$ cents ($\approx \$9.22 \times 10^{16}$), comfortably exceeding global equities capitalizations.

---

## 5. Price Improvement Algorithm

When an aggressive incoming order crosses the spread, at what price does the trade execute?

ExchangeCore implements the exchange industry standard: **resting order price priority (price improvement)**.

### Example Walkthrough
1. **Resting Book**: Sell 100 shares of AAPL resting at **\$150.00**.
2. **Incoming Order**: Client submits Buy 100 shares with Limit price **\$152.00**.
3. **Execution Logic**:
   - The buyer stated: *"I am willing to pay up to \$152.00."*
   - The seller stated: *"I am willing to sell for \$150.00."*
   - **Trade Price = \$150.00** (the resting order price).
   - The buyer receives **\$2.00 per share in price improvement**.

```cpp
Price trade_price = resting_order->price; // Always executed at resting price
```

---

## 6. How Production HFT Engines Differ (Roadmap to Ultra Low-Latency)

While ExchangeCore uses STL containers for clarity, modularity, and correctness, ultra-high-frequency production engines implement specialized memory layouts:

```
┌─────────────────────────────────────────────────────────────┐
│ Production HFT Engine                                       │
├─────────────────────────────────────────────────────────────┤
│ 1. Flat Cache-Friendly B-Trees / Radix Trees                │
│    (Nodes packed into contiguous 64-byte CPU cache lines)   │
├─────────────────────────────────────────────────────────────┤
│ 2. Pre-Allocated Object Memory Pools                        │
│    (Zero heap allocations / malloc during hot trading loop) │
├─────────────────────────────────────────────────────────────┤
│ 3. Intrusive Doubly-Linked Lists                            │
│    (Pointers embedded directly inside Order struct)         │
├─────────────────────────────────────────────────────────────┤
│ 4. Kernel-Bypass Networking (Solarflare OpenOnload, DPDK)   │
│    (Packets copied directly from NIC ring buffer to userspace)│
└─────────────────────────────────────────────────────────────┘
```

ExchangeCore purposely isolates these concerns so you can master the matching fundamentals first before tackling cache optimization and assembly-level tuning.
