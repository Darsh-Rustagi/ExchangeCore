# ExchangeCore — Market Depth Dashboard

The **ExchangeCore Dashboard** is a Python analytics and visualization tool designed to reconstruct order book states and plot market depth, trade prices, and cumulative volumes directly from the matching engine's JSON Lines event log.

---

## Features

- **Order Book Depth Chart**: Visualizes bid levels (green) and ask levels (red) with share volume at each price level.
- **Trade Execution Timeline**: Real-time scatter and line plot showing executed trade prices and fill sizes.
- **Cumulative Traded Volume**: Growth of total traded shares over the trading session.
- **Summary Performance Metrics**: Total orders processed, cancellation rate, fill ratio, volume-weighted average price (VWAP), and inside spread.

---

## Setup & Requirements

Install dependencies using Python 3.10+:

```bash
pip install -r dashboard/requirements.txt
```

---

## Running the Dashboard

### 1. Generate an Event Log from the Matching Engine
First, run ExchangeCore to replay or generate events to a JSON Lines file:

```bash
./build/exchange_main --replay data/sample_orders.jsonl data/sample_events.jsonl
```

### 2. Visualize or Export Charts

**Save as high-resolution image:**
```bash
python3 dashboard/dashboard.py --input data/sample_events.jsonl --save docs/diagrams/market_depth_dashboard.png
```

**Display interactively on screen:**
```bash
python3 dashboard/dashboard.py --input data/sample_events.jsonl
```

**Custom price levels and symbol filter:**
```bash
python3 dashboard/dashboard.py --input data/sample_events.jsonl --symbol AAPL --levels 20
```

---

## Command-Line Arguments

| Flag | Default | Description |
|---|---|---|
| `--input`, `-i` | `data/sample_events.jsonl` | Path to event log recorded by ExchangeCore |
| `--save`, `-s` | `None` (Interactive display) | Path to output PNG image |
| `--symbol` | `AAPL` | Symbol to filter and reconstruct |
| `--levels` | `15` | Maximum number of price levels to display on depth chart |
