#include <stdint.h>
#include <assert.h>
#include <limits.h>

typedef struct {
	uint64_t *data;
	int word_count;
	int min_slot;
	int max_slot;
	uint64_t used_count;
} bitlog_t;



static int bitlog_word_count_for_bits(uint64_t bits)
{
	// 1. Calculate required 64-bit words safely avoiding integer overflow
	uint64_t words = (bits >> 6) + ((bits & 63) != 0);
	
	// 2. Reject values that would overflow an int
	if (words > INT_MAX) return -1;
	
	return (int)words;
}



static int bitlog_is_empty(const bitlog_t *log)
{
	// 1. Evaluate O(1) truth from used count
	return log->used_count == 0;
}



static int bitlog_init(bitlog_t *log, uint64_t *buffer, int word_count)
{
	// 1. Validate inputs
	if (!log || !buffer || word_count <= 0) return -1;

	// 2. Assign memory and capacity
	log->data = buffer;
	log->word_count = word_count;
	log->used_count = 0;
	
	// 3. Set cursors to default empty state
	log->min_slot = word_count;
	log->max_slot = -1;

	// 4. Scan existing buffer to build accurate hints and count
	for (int i = 0; i < word_count; i++) {
		uint64_t w = buffer[i];
		if (!w) continue;
		
		if (log->min_slot == word_count) log->min_slot = i;
		log->max_slot = i;
		log->used_count += __builtin_popcountll(w);
	}

	return 0;
}



static int bitlog_test(const bitlog_t *log, uint64_t index)
{
	uint64_t slot64 = index >> 6;
	
	// 1. Validate bounds safely
	assert(slot64 < (uint64_t)log->word_count);
	
	// 2. Evaluate bit state
	return (log->data[(int)slot64] & (1ULL << (index & 63))) != 0ULL;
}



static void bitlog_set(bitlog_t *log, uint64_t index)
{
	uint64_t slot64 = index >> 6;
	
	// 1. Validate bounds safely
	assert(slot64 < (uint64_t)log->word_count);
	
	int slot = (int)slot64;
	uint64_t mask = 1ULL << (index & 63);
	
	// 2. Set the targeted bit and increment usage if not previously set
	if (!(log->data[slot] & mask)) {
		log->data[slot] |= mask;
		log->used_count++;
	}
	
	// 3. Expand search hints
	if (slot < log->min_slot) log->min_slot = slot;
	if (slot > log->max_slot) log->max_slot = slot;
}



static void bitlog_clear(bitlog_t *log, uint64_t index)
{
	uint64_t slot64 = index >> 6;
	
	// 1. Validate bounds safely
	assert(slot64 < (uint64_t)log->word_count);
	
	int slot = (int)slot64;
	uint64_t mask = 1ULL << (index & 63);
	
	// 2. Clear bit and decrement usage if previously set
	if (log->data[slot] & mask) {
		log->data[slot] &= ~mask;
		log->used_count--;
	}
	
	// 3. Perfect empty state snap
	if (log->used_count == 0) {
		log->min_slot = log->word_count;
		log->max_slot = -1;
		return;
	}
	
	// 4. Tighten min_slot if we cleared the lowest boundary
	if (slot == log->min_slot) {
		while (log->min_slot <= log->max_slot && !log->data[log->min_slot])
			log->min_slot++;
	}
	
	// 5. Tighten max_slot if we cleared the highest boundary
	if (slot == log->max_slot) {
		while (log->max_slot >= log->min_slot && !log->data[log->max_slot])
			log->max_slot--;
	}
}



static int bitlog_pop_lowest(bitlog_t *log)
{
	// 1. Validate state
	if (bitlog_is_empty(log)) return -1;
	
	// 2. Scan from lowest known populated slot
	for (int i = log->min_slot; i <= log->max_slot; i++) {
		uint64_t w = log->data[i];
		
		if (w) {
			// 3. Clear lowest bit and decrement usage
			int b = __builtin_ctzll(w);
			log->data[i] = w & (w - 1);
			log->used_count--;
			
			// 4. Perfect empty state snap
			if (log->used_count == 0) {
				log->min_slot = log->word_count;
				log->max_slot = -1;
				return (i << 6) + b;
			}
			
			// 5. Tighten cursor
			if (log->data[i]) log->min_slot = i;
			else log->min_slot = i + 1;
			
			return (i << 6) + b;
		}
	}
	
	return -1;
}



static int bitlog_pop_highest(bitlog_t *log)
{
	// 1. Validate state
	if (bitlog_is_empty(log)) return -1;
	
	// 2. Scan backward from highest known populated slot
	for (int i = log->max_slot; i >= log->min_slot; i--) {
		uint64_t w = log->data[i];
		
		if (w) {
			// 3. Clear highest bit and decrement usage
			int b = 63 - __builtin_clzll(w);
			log->data[i] = w & ~(1ULL << b);
			log->used_count--;
			
			// 4. Perfect empty state snap
			if (log->used_count == 0) {
				log->min_slot = log->word_count;
				log->max_slot = -1;
				return (i << 6) + b;
			}
			
			// 5. Tighten cursor
			if (log->data[i]) log->max_slot = i;
			else log->max_slot = i - 1;
			
			return (i << 6) + b;
		}
	}
	
	return -1;
}



static uint64_t bitlog_get_used_count(const bitlog_t *log)
{
	// 1. Return O(1) truth
	return log->used_count;
}



static void bitlog_clear_all(bitlog_t *log)
{
	// 1. Reset cursors and usage count
	log->min_slot = log->word_count;
	log->max_slot = -1;
	log->used_count = 0;
	
	// 2. Zero memory
	for (int i = 0; i < log->word_count; i++)
		log->data[i] = 0ULL;
}