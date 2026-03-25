<img src="icon.svg" width="80">

# bots

![version](https://img.shields.io/badge/version-v1.0.0-blue)

Automation bots for phone calls, food ordering, and inventory tracking.

## Bots

| Bot | Description | Status |
|-----|-------------|--------|
| **fony** | AI phone calls via Twilio TTS | Working |
| **food** | Automated food ordering (7 chains) | Working |
| **starbucks** | Starbucks ordering | Blocked (needs mitmproxy) |
| **weedbot** | Cannabis strain tracker | Working |

## Run

```bash
node food/food.js <command>       # usual, place, menu, track, stores, loyalty, coupons
node --test weedbot/test.js       # 40 tests
```

## License

MIT 2026 Joshua Trommel (via systems/ root)
