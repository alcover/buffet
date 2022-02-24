#ifndef ALCO_UTIL_H
#define ALCO_UTIL_H

static inline unsigned int 
nextlog2 (unsigned long long n) {
	return 8 * sizeof(unsigned long long) - __builtin_clzll(n-1);
}

#endif