# bitlog.h

Fixed-size bit array tracking system for C99.

`bitlog` is a single-header, zero-allocation utility designed to manage a compact bit array backed by a user-provided buffer of 64-bit words. 

It provides constant-time `O(1)` metadata tracking, including population counts and active slot boundaries. The internal tracking structures minimize scanning overhead during prioritized extractions, making it highly suitable for high-performance scheduling, tracking, or event logs.

## Features

* **Zero Allocation:** Operates entirely on user-provided memory buffers.
* **Highly Optimized:** Utilizes hardware instructions (`__builtin_ctzll`, `__builtin_clzll`) for rapid bit extraction.
* **O(1) Metrics:** Instant access to usage counts and empty-state checks.
* **Single-Header:** Drop-in integration with no complex build steps.
* **LTO Ready:** Designed for aggressive compiler inlining.

---

## Integration & Performance

The functions are implemented with internal linkage using the `static` keyword. Simply `#include "bitlog.h"` directly in your standard C files. 

For maximum performance, compile with:

```bash
clang -O3 -flto

```

This configuration allows Clang to aggressively inline runtime operations directly into your loops and optimize away unused functions via Link-Time Optimization (LTO).

---

## Quick Start

```c
#include <stdio.h>
#include "bitlog.h"

int main(void)
{
	// 1. Allocate your backing buffer (e.g., 4 words = 256 bits)
	uint64_t buffer[4] = {0};
	bitlog_t log;

	// 2. Initialize the bitlog
	if (bitlog_init(&log, buffer, 4) != 0) {
		return -1;
	}

	// 3. Set some bits
	bitlog_set(&log, 12);
	bitlog_set(&log, 150);

	// 4. Retrieve and clear the highest prioritized bit
	int highest = bitlog_pop_highest(&log);
	printf("Highest bit was: %d\n", highest); // Outputs 150
	
	// 5. Check remaining usage
	printf("Bits remaining: %llu\n", bitlog_get_used_count(&log)); // Outputs 1

	return 0;
}

```

---

## API Reference

### Data Structures

#### `bitlog_t`

The core tracking structure. Should be initialized via `bitlog_init`.

```c
typedef struct {
	uint64_t *data;
	int word_count;
	int min_slot;
	int max_slot;
	uint64_t used_count;
} bitlog_t;

```

### Initialization & Utility

* **`int bitlog_word_count_for_bits(uint64_t bits)`**
Calculates the number of 64-bit words required to safely hold the specified number of `bits` without integer overflow. Returns the word count, or `-1` on overflow.
* **`int bitlog_init(bitlog_t *log, uint64_t *buffer, int word_count)`**
Initializes the `bitlog_t` structure to manage the provided memory `buffer`. Scans existing data to accurately construct internal tracking hints and population counts. Returns `0` on success, or `-1` if inputs are invalid.
* **`void bitlog_clear_all(bitlog_t *log)`**
Resets all data words in the backing buffer to `0` and clears all internal cursors and metrics.

### Bit Manipulation

* **`void bitlog_set(bitlog_t *log, uint64_t index)`**
Sets the bit at the specified `index` to `1`. Tracks internal boundary changes and increments the total population count if the bit was not previously set. Enforces bounds via `assert()`.
* **`void bitlog_clear(bitlog_t *log, uint64_t index)`**
Clears the bit at the specified `index` to `0`. Dynamically tightens search boundaries and decrements tracking counts. Enforces bounds via `assert()`.
* **`int bitlog_test(const bitlog_t *log, uint64_t index)`**
Returns non-zero if the bit at the specified `index` is set, and `0` otherwise. Enforces bounds via `assert()`.

### Extraction & Querying

* **`int bitlog_pop_lowest(bitlog_t *log)`**
Finds, clears, and returns the absolute bit index of the **lowest** set bit in the log. Returns `-1` if the log is empty.
* **`int bitlog_pop_highest(bitlog_t *log)`**
Finds, clears, and returns the absolute bit index of the **highest** set bit in the log. Returns `-1` if the log is empty.
* **`int bitlog_is_empty(const bitlog_t *log)`**
Returns a non-zero value if the log contains no set bits, and `0` otherwise. (`O(1)`)
* **`uint64_t bitlog_get_used_count(const bitlog_t *log)`**
Returns the total number of bits currently set in the log. (`O(1)`)

---

## License

Conforms to standard C99 requirements. Free to use and modify.

```

```