# Binary Protocol Specification (OrderCommand)

**Protocol name:** OBK1 (Order Book wire format, version 1)  
**Status:** Frozen for Milestone 5A — encoder/decoder not implemented yet  
**Canonical endianness:** Little-endian for all multi-byte numeric fields

## Purpose

This document defines a **fixed-width, little-endian** binary representation of `matching_engine::OrderCommand` for:

- Lower-overhead command files than CSV
- Future Milestone 5D+ binary file readers and Milestone 7 gateways
- Deterministic, testable byte layouts without changing `MatchingEngine` logic

The binary layer sits **between** I/O and the engine:

```text
bytes on disk / wire  →  [encoder/decoder — Milestone 5B+]  →  OrderCommand  →  MatchingEngine
```

## Scope of Milestone 5A

**In scope (this document + `include/protocol/BinaryProtocol.hpp`):**

- Frozen field layout, sizes, and offsets
- Enum wire values and validation rules
- Forward-compatibility notes

**Out of scope (later sub-milestones):**

- `encode()` / `decode()` implementations (5B, 5C)
- Binary file reader (5D)
- CLI `--binary-engine` (5E)
- Networking, FIX, ITCH, compression, encryption

## Non-goals / disclaimer

This protocol is for **simulated exchange and order-command replay** in an educational C++ project. It is **not** a production trading connectivity standard and must not be used for real-money trading without independent review.

## Message model

- One **OrderCommand message** = exactly **64 bytes**
- Messages are stored **back-to-back** in binary files (no per-message length prefix in v1)
- File size must be a multiple of 64; otherwise the reader rejects the file (5D)
- An empty binary command file is valid and reads as zero commands (5D)
- Local binary engine replay uses `--binary-engine <file.obk>` (5E); this is simulated replay only, not exchange connectivity.
- Binary command files can be read either buffered (all decoded commands returned in a vector) or streaming (one 64-byte message decoded and passed to a callback immediately); both paths use the same fixed wire layout and decoder.

There is **no** separate variable-length payload in version 1.

## Relationship to `OrderCommand`

| `OrderCommand` field | Binary field | Notes |
|----------------------|--------------|--------|
| `type` | `command_type` | `NewOrder`, `CancelOrder`, `ModifyOrder` |
| `order_id` | `order_id` | `uint64_t` |
| `side` | `side` | `market_data::Side` |
| `order_type` | `order_type` | `Limit`, `Market` |
| `price` | `price` | `int64_t` ticks/cents |
| `quantity` | `quantity` | `uint64_t` |
| `symbol` | `symbol` | Fixed 16-byte ASCII; maps to `std::string` |
| *(not in struct today)* | `timestamp` | Wire-only metadata for ordering/replay; decoder may ignore or pass through in 5B+ |

`MatchingEngine` behaviour is unchanged in 5A; timestamp does not affect matching until explicitly wired in a later milestone.

---

## Byte layout (64 bytes)

All offsets are zero-based.

| Offset | Size | Field | Type (logical) | Description |
|--------|------|--------|----------------|-------------|
| 0 | 4 | `magic` | `uint32_t` | Must be `0x314B424F` (ASCII `OBK1` as LE uint32) |
| 4 | 1 | `version` | `uint8_t` | Protocol version; must be `1` for this spec |
| 5 | 1 | `wire_message_type` | `uint8_t` | Must be `0x01` (`OrderCommandMessage`) |
| 6 | 2 | `header_reserved` | `uint8_t[2]` | Must be `0x00`; ignore on read |
| 8 | 8 | `timestamp` | `uint64_t` | Nanoseconds or application ticks; encoder-defined |
| 16 | 1 | `command_type` | `uint8_t` | See [Command type](#command-type-command_type) |
| 17 | 1 | `side` | `uint8_t` | See [Side](#side-side) |
| 18 | 1 | `order_type` | `uint8_t` | See [Order type](#order-type-order_type) |
| 19 | 1 | `payload_reserved` | `uint8_t` | Must be `0x00`; ignore on read |
| 20 | 4 | `align_reserved` | `uint8_t[4]` | Must be `0x00`; ignore on read |
| 24 | 8 | `order_id` | `uint64_t` | Client order identifier |
| 32 | 8 | `price` | `int64_t` | Limit price in ticks/cents; `0` allowed for market |
| 40 | 8 | `quantity` | `uint64_t` | Order quantity; `0` only where engine allows |
| 48 | 16 | `symbol` | `char[16]` | ASCII symbol, space- or NUL-padded |

**Total size:** 64 bytes (`protocol::kMessageSize`).

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                            magic                              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| ver | wt  |      header_reserved      |                       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                         timestamp                             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| ct  | side| ot  |pr |           align_reserved                |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                          order_id                             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           price                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                          quantity                             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                          symbol (16)                          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

---

## Header fields

### `magic` (4 bytes)

- Value: **`0x314B424F`** (little-endian uint32)
- Byte sequence on wire: `4F 42 4B 31` (`'O' 'B' 'K' '1'`)
- Wrong magic → **reject** message (decoder 5C+)

### `version` (1 byte)

- Value: **`1`** for this specification
- Unknown version → **reject** (do not guess layout)

### `wire_message_type` (1 byte)

Distinguishes top-level message kinds for future protocol extension.

| Value | Name | Meaning |
|-------|------|---------|
| `0x01` | `OrderCommandMessage` | Payload is OrderCommand fields below |
| `0x00`, `0x02`–`0xFE` | — | Reserved; **reject** in v1 |
| `0xFF` | — | Invalid; **reject** |

Command semantics (`NEW` / `CANCEL` / `MODIFY`) are in `command_type`, not here.

### `header_reserved` (2 bytes)

- Must be zero
- Non-zero → **reject** in v1 (allows future header flags if version bumped)

---

## Payload fields

### `timestamp` (8 bytes)

- `uint64_t`, little-endian
- Meaning: application-defined (e.g. event time in nanoseconds, or CSV replay timestamp)
- Not used by `MatchingEngine` in Milestone 5A
- Decoder may store for logging or future `Order::timestamp` assignment (5B+ decision)

### Command type (`command_type`)

Maps to `matching_engine::OrderCommandType`.

| Value | Name | `OrderCommandType` |
|-------|------|---------------------|
| `0x01` | `NewOrder` | `NewOrder` |
| `0x02` | `CancelOrder` | `CancelOrder` |
| `0x03` | `ModifyOrder` | `ModifyOrder` |
| `0x00`, `0x04`–`0xFE` | Reserved | **Reject** |
| `0xFF` | Invalid | **Reject** |

### Side (`side`)

Maps to `market_data::Side`.

| Value | Name | `Side` |
|-------|------|--------|
| `0x01` | `Buy` | `BUY` |
| `0x02` | `Sell` | `SELL` |
| `0x00` | `Unknown` | `UNKNOWN` (e.g. cancel rows) |
| `0x03`–`0xFE` | Reserved | **Reject** |
| `0xFF` | Invalid | **Reject** |

**Validation (encoder + decoder):**

- `NewOrder` / `ModifyOrder`: side must be `Buy` or `Sell`
- `CancelOrder`: side may be `Unknown`; if `Buy`/`Sell` is present, must match existing order on modify paths (engine rule)

### Order type (`order_type`)

Maps to `matching_engine::OrderType`.

| Value | Name | `OrderType` |
|-------|------|-------------|
| `0x01` | `Limit` | `Limit` |
| `0x02` | `Market` | `Market` |
| `0x00`, `0x03`–`0xFE` | Reserved | **Reject** |
| `0xFF` | Invalid | **Reject** |

### `payload_reserved` / `align_reserved`

- Must be zero in v1
- Non-zero → **reject**

### `order_id` (8 bytes)

- `uint64_t`, little-endian
- Must be non-zero for `NewOrder` (encoder validation, 5B+)

### `price` (8 bytes)

- `int64_t`, little-endian, ticks/cents (same as CSV)
- `Limit`: must be `> 0` for `NewOrder` / `ModifyOrder` unless engine policy changes
- `Market`: should be `0` (CSV uses `0` placeholder)

### `quantity` (8 bytes)

- `uint64_t`, little-endian
- `NewOrder` / `ModifyOrder`: must be `> 0` and `<= UINT32_MAX` (engine limit)
- `CancelOrder`: may be `0` (ignored by engine)

### `symbol` (16 bytes)

- Fixed-length ASCII
- Padding: `0x00` (NUL) or `0x20` (space) after left-aligned text only
- No UTF-8 multi-byte in v1
- Decoder: left-aligned text (printable ASCII `0x21`–`0x7E`), then padding of NUL/space only; **reject** any other byte in the field or non-padding after the first padding byte
- Decoder trims trailing NULs and spaces → `std::string`
- Empty symbol (all padding) → **reject** for `NewOrder` / `ModifyOrder`
- `CancelOrder` with empty symbol (all NUL/space padding) → decoder maps to `OrderCommand` default `"DEFAULT"` (engine ignores symbol on cancel)
- Shorter names are left-aligned: `"AAPL"` → `41 41 50 4C 00 ...` or `41 41 50 4C 20 20 ...`

---

## Encoding rules for enums

1. Store as **single unsigned byte** at defined offset.
2. Only values listed above are valid for v1.
3. Reserved values must not be emitted by encoders.
4. Decoders must **reject** unknown enum bytes (no silent coercion).
5. C++ `enum class` values in application code are **not** written directly; use `protocol::*Wire` constants from `BinaryProtocol.hpp`.

---

## Validation rules (decoder / encoder — Milestone 5B+)

| Check | Action on failure |
|-------|-------------------|
| Buffer size ≠ 64 | Reject |
| `magic` mismatch | Reject |
| `version` ≠ 1 | Reject |
| `wire_message_type` ≠ `0x01` | Reject |
| Any reserved byte non-zero | Reject |
| Invalid `command_type` / `side` / `order_type` | Reject |
| `symbol` padding not NUL/space, or garbage after first padding byte | Reject |
| `NewOrder` with invalid side/qty/price/symbol | Reject |
| `ModifyOrder` with invalid qty/price | Reject |
| Trailing garbage after message in fixed file | Reject file (size % 64 ≠ 0) |

Rejections should surface a **clear error string**; no undefined behaviour.

---

## Forward compatibility

- **Version bump:** New layout only with `version > 1`. v1 decoders reject higher versions.
- **Reserved bytes:** Must be written as zero; decoders reject non-zero in v1 so flags can use those bytes in v2.
- **`wire_message_type`:** New message kinds (e.g. heartbeat) get new type IDs without overloading `command_type`.
- **Symbol length:** Increasing past 16 bytes requires new version (breaking change).
- **File framing:** v1 uses fixed 64-byte records; a future version may add a file header or length prefix in a separate container format.

---

## Example: logical command → binary fields

**Logical command (CSV-equivalent):**

```text
NEW, order_id=1, SELL, LIMIT, price=10055, quantity=100, symbol="AAPL"
timestamp=1001 (application units)
```

**Field values:**

| Field | Value |
|-------|--------|
| `magic` | `0x314B424F` |
| `version` | `1` |
| `wire_message_type` | `0x01` |
| `header_reserved` | `00 00` |
| `timestamp` | `1001` → `E9 03 00 00 00 00 00 00` |
| `command_type` | `0x01` (NewOrder) |
| `side` | `0x02` (Sell) |
| `order_type` | `0x01` (Limit) |
| `payload_reserved` | `00` |
| `align_reserved` | `00 00 00 00` |
| `order_id` | `1` → `01 00 00 00 00 00 00 00` |
| `price` | `10055` → `47 27 00 00 00 00 00 00` |
| `quantity` | `100` → `64 00 00 00 00 00 00 00` |
| `symbol` | `41 41 50 4C` + NUL pad |

**Full message (hex, 64 bytes):**

```text
4F 42 4B 31  01 01 00 00  E9 03 00 00 00 00 00 00
01 02 01 00  00 00 00 00  01 00 00 00 00 00 00 00
47 27 00 00 00 00 00 00  64 00 00 00 00 00 00 00
41 41 50 4C 00 00 00 00 00 00 00 00 00 00 00 00
```

*(Verify on target platform when implementing encoder tests in 5B.)*

---

## Invariants (encoder / decoder must preserve)

1. **Fixed size:** Every message is exactly 64 bytes.
2. **Little-endian:** All `uint16`/`uint32`/`uint64`/`int64` fields use LE byte order.
3. **Magic and version:** Every valid message has correct `magic` and `version == 1`.
4. **Zero reserved:** All reserved fields are zero in v1.
5. **Enum mapping:** Wire enums map 1:1 to application enums as tables above; no overlapping meanings.
6. **No padding surprises:** Layout uses explicit offsets; do not use compiler-packed structs without `static_assert` checks against this table.
7. **OrderCommand semantics:** Decoded commands must be acceptable to `MatchingEngine::process` under the same rules as `OrderCommandParser` CSV input.
8. **Determinism:** Encoding the same logical command with the same timestamp and symbol padding yields **identical** 64 bytes.
9. **Isolation:** Protocol code must not call `MatchingEngine` or mutate `OrderBook`.
10. **CSV coexistence:** Binary and CSV paths produce equivalent engine state when given equivalent command sequences (verified in 5E integration tests).

---

## Reference constants

See `include/protocol/BinaryProtocol.hpp` for `constexpr` offsets, sizes, magic, version, and `static_assert` checks. Encoder and decoder implementations belong in Milestone 5B and 5C.
