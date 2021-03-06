#ifndef __SHORT_DICT_MATCHING__
#define __SHORT_DICT_MATCHING__

#include "karp_rabin.h"
#include "hash_lookup.h"

#define GET_INDEX(k, m, start) ((start + (k << 1) - m) % (k << 1))

typedef struct short_dict_matcher_t {
    hash_lookup lookup;
    int num_prints;
    int k;
    int k_p;
} *short_dict_matcher;

int suffix(char *t, int n, char **p, int *m, int num_patterns) {
    int i, j, diff;
    for (i = 0; i < num_patterns; i++) {
        diff = n - m[i];
        if (diff >= 0) {
            for (j = 0; j < m[i]; j++) {
                if (t[diff + j] != p[i][j]) {
                    break;
                }
            }
            if (j == m[i]) {
                return 1;
            }
        }
    }
    return 0;
}

short_dict_matcher short_dict_matching_build(int k, fingerprinter printer, char **p, int *m) {
    short_dict_matcher state = malloc(sizeof(struct short_dict_matcher_t));
    int i, j, k_p = 1;
    while (k_p < (k << 1)) {
        k_p <<= 1;
    }
    state->k = k;
    state->k_p = k_p;
    fingerprint *pattern_prints = malloc(sizeof(fingerprint) * k_p * k_p);
    int *suffix_match = malloc(sizeof(int) * k_p * k_p);
    for (i = 0; i < k_p * k_p; i++) {
        pattern_prints[i] = init_fingerprint();
        suffix_match[i] = 0;
    }

    int num_prints = 0, x, y;
    for (i = 0; i < k; i++) {
        if (m[i] <= (k << 1)) {
            x = k_p >> 1;
            y = m[i];
            while (x != 0) {
                if (y >= x) {
                    set_fingerprint(printer, &p[i][y - x], m[i] - y + x, pattern_prints[num_prints]);
                    suffix_match[num_prints] = suffix(&p[i][y - x], m[i] - y + x, p, m, k);
                    for (j = 0; j < num_prints; j++) {
                        if (fingerprint_equals(pattern_prints[num_prints], pattern_prints[j])) {
                            suffix_match[j] |= suffix_match[num_prints];
                            break;
                        }
                    }
                    if (j == num_prints) {
                        num_prints++;
                    }
                    y -= x;
                }
                x >>= 1;
            }
            set_fingerprint(printer, p[i], m[i], pattern_prints[num_prints]);
            suffix_match[num_prints] = 1;
            for (j = 0; j < num_prints; j++) {
                if (fingerprint_equals(pattern_prints[num_prints], pattern_prints[j])) {
                    suffix_match[j] |= suffix_match[num_prints];
                    break;
                }
            }
            if (j == num_prints) {
                num_prints++;
            }
        }
    }

    state->num_prints = num_prints;
    if (num_prints) state->lookup = hashlookup_build(pattern_prints, suffix_match, num_prints, printer);

    for (i = 0; i < k_p * k_p; i++) {
        fingerprint_free(pattern_prints[i]);
    }
    free(pattern_prints);
    free(suffix_match);

    return state;
}

int short_dict_matching_stream(short_dict_matcher state, fingerprinter printer, fingerprint t_f, fingerprint *t_prev, fingerprint tmp, int j) {
    int result = -1, found, match = 0, start = 0, end = state->k_p, middle;
    if (state->num_prints) {
        while (start < end) {
            middle = (start + end) / 2;
            if (middle <= (state->k << 1)) {
                fingerprint_suffix(printer, t_f, t_prev[GET_INDEX(state->k, middle, j)], tmp);
                found = hashlookup_search(state->lookup, tmp, &match);
                if (match) {
                    result = j;
                    break;
                } else if (found == -1) {
                    end = middle;
                } else {
                    start = middle + 1;
                }
            } else {
                end = middle;
            }
        }
        if (result == -1) {
            middle = start;
            if (middle <= (state->k << 1)) {
                fingerprint_suffix(printer, t_f, t_prev[GET_INDEX(state->k, middle, j)], tmp);
                found = hashlookup_search(state->lookup, tmp, &match);
                if (match) {
                    result = j;
                }
            }
        }
    }

    return result;
}

void short_dict_matching_free(short_dict_matcher state) {
    if (state->num_prints) hashlookup_free(&state->lookup);
    free(state);
}

int short_dict_matching_size(short_dict_matcher state) {
    int size = sizeof(struct short_dict_matcher_t);
    if (state->num_prints) size += hashlookup_size(state->lookup);
    return size;
}

#endif
