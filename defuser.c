#include "defuser.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define MAX_FIELDS 8

/*
 * 把 values 中的 count 个整数写成一行“空格分隔的无符号十进制整数”，存入 out。
 * 成功返回 0；count 为 0（还没有实现）、snprintf 出错或结果被截断时返回非零。
 */
static int write_answer(char *out, size_t cap, const uint32_t *values, size_t count) {
    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        int size = snprintf(out + used, cap - used, "%s%" PRIu32, i ? " " : "", values[i]);
        if (size < 0 || (size_t)size >= cap - used) return 1;
        used += (size_t)size;
    }
    return count == 0;
}

typedef struct {
    unsigned next;       // 下一个节点在 nodes[] 中的下标
    uint32_t field8;     // 原节点地址 +8 处的值
    uint32_t field12;    // 原节点地址 +12 处的值
} P5Node;

static const P5Node nodes[12] = {
    /* 每项填写 next 的节点下标、field8、field12 */
    [0]  = { .next = 4,  .field8 = 0x0576, .field12 = 0x36a6 },
    [1]  = { .next = 7,  .field8 = 0x0034, .field12 = 0x22f8 },
    [2]  = { .next = 5,  .field8 = 0x046e, .field12 = 0x59d4 },
    [3]  = { .next = 6,  .field8 = 0x0b6a, .field12 = 0x1a49 },
    [4]  = { .next = 2,  .field8 = 0x0650, .field12 = 0x6567 },
    [5]  = { .next = 12, .field8 = 0x024a, .field12 = 0xf3c6 }, // 原 next 为 NULL
    [6]  = { .next = 1,  .field8 = 0x015a, .field12 = 0x447e },
    [7]  = { .next = 8,  .field8 = 0x0865, .field12 = 0x59f8 },
    [8]  = { .next = 11, .field8 = 0x0351, .field12 = 0xc404 },
    [9]  = { .next = 0,  .field8 = 0x096e, .field12 = 0xc155 },
    [10] = { .next = 3,  .field8 = 0x0a2f, .field12 = 0x66bc },
    [11] = { .next = 9,  .field8 = 0x077f, .field12 = 0x736c },
};

static const unsigned heads[8] = {10, 3, 6, 1, 7, 8, 11, 9};
static uint32_t walk_phase4(uint32_t target, uint32_t low,uint32_t high, uint32_t salt) {
    
    uint32_t mid = (low + high) / 2;
    if(mid == target){
        return (mid + 0x54) ^ salt;
    } else if(mid < target){
        uint32_t child = walk_phase4(target, mid + 1, high, salt);
        return (3 * child) ^ (mid + salt) ^ 0x9;
    } else {
        uint32_t child = walk_phase4(target, low, mid - 1, salt);
        return (3u * child) ^ (mid + salt) ^ 0x5u;
    }
}

int solve_phase_1(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;
    (void)challenge;
    // TODO: 根据 phase_01 的汇编，用 challenge 算出口令中的各个整数，依次存入 values，并把 count 改为整数个数。
    uint32_t low_32 = (uint32_t)(challenge & 0xFFFFFFFF);
    uint32_t high_32 = (uint32_t)((challenge >> 32) & 0xFFFFFFFF);
    uint32_t step_2 = (uint32_t)((low_32 + 0x8c5b) ^ high_32);
    uint32_t shr_16 = (uint32_t)(challenge >> 16);
    uint32_t step_0 = (uint32_t)((3 * low_32 + shr_16) ^ 0x1e96);
    uint32_t step_1 = (uint32_t)(5 * step_0 ^ (step_2 + 0xbfc9));
    values[0] = step_1;
    count = 1;

    return write_answer(out, cap, values, count);
}

int solve_phase_2(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;
    (void)challenge;
    // TODO: 根据 phase_02 的汇编，用 challenge 算出口令中的各个整数，依次存入 values，并把 count 改为整数个数。
    values[0] = (uint32_t)(((challenge >> 20) & 0xFF) * 3u);
    values[1] = ((((challenge >> 24) & 0x3Fu) + 1u) ^ values[0]) * 5u;
    values[2] = (uint32_t)((((challenge >> 28) & 0x3Fu) + 2u) ^ values[1]) * 5u;
    values[3] = (uint32_t)((((challenge >> 32) & 0x3Fu) + 3u) ^ values[2]) * 5u;
    values[4] = (uint32_t)((((challenge >> 36) & 0x3Fu) + 4u) ^ values[3]) * 5u;
    values[5] = (uint32_t)((((challenge >> 40) & 0x3Fu) + 5u) ^ values[4]) * 5u;
    count = 6;
    return write_answer(out, cap, values, count);
}

int solve_phase_3(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;
    (void)challenge;
    // TODO: 根据 phase_03 的汇编，用 challenge 算出口令中的各个整数，依次存入 values，并把 count 改为整数个数。
    uint32_t b = (uint32_t)((challenge >> 3) ^ (challenge >> 32));
    uint32_t a = (uint32_t)(challenge & 0x7);
    switch (a)
    {
    case 0:
        values[1] = 0xbc76 + 5 * b;
        break;
    case 1:
        values[1] = 0x20ea ^ (4 * b);
        break;
    case 2:
        values[1] = (b >> 1) + (b ^ 0x8d36);
        break;
    case 3:
        values[1] = (b >> 2) + (b ^ 0xa1c);
        break;
    case 4:
        values[1] = (5 * b) + 0xcca0;
        break;
    case 5:
        values[1] = (2 * b) ^ 0x445b;
        break;
    case 6:
        values[1] = (5 * b) + 0x32c53;
        break;
    case 7:
        values[1] = (5 * b) + 0x47ced;
        break;
    default:
        values[1] = 0; // This case should never happen, but we set a default value to avoid uninitialized variable warning.
        break;
    }
    values[0] = a;
    count = 2;
    return write_answer(out, cap, values, count);
}

int solve_phase_4(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;
    (void)challenge;
    // TODO: 根据 phase_04 的汇编，用 challenge 算出口令中的各个整数，依次存入 values，并把 count 改为整数个数。
    uint32_t low7 = (uint32_t)(challenge & 0x7F);
    uint32_t x = (uint32_t)((challenge >> 7) ^ (challenge >> 32));
    values[0] = walk_phase4(low7, 0, 127, x);
    count = 1;
    return write_answer(out, cap, values, count);
}

int solve_phase_5(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;
    (void)challenge;
    // TODO: 根据 phase_05 的汇编，用 challenge 算出口令中的各个整数，依次存入 values，并把 count 改为整数个数。
    uint32_t start = challenge  & 0x7;
    uint32_t hops = (challenge >> 3) & 0x3;
    unsigned current = heads[start];
    uint32_t acc = ((challenge) >> 6) ^ ((challenge) >> 0x20) ^ 0xcb34;
    for(uint32_t step = 0;step <= hops; step++){
        const P5Node *node = &nodes[current];
        acc = (3 * acc + node->field12) ^ node->field8;
        
        values[1] = node->field8;
        if(node->next == 12){
            break;
        }
        current = node->next;
    }
    values[0] = acc;
    count = 2;
    return write_answer(out, cap, values, count);
}

uint32_t p6_ops_0(uint32_t x) {
    return (x >> 3) ^ (x + 0x7826);
}
uint32_t p6_ops_1(uint32_t x) {
    return (x ^ 0x8854) + 8 * x;
}
uint32_t p6_ops_2(uint32_t x) {
    return ((x << 5) + 0x2370) ^ (7 * x);
}
uint32_t p6_ops_3(uint32_t x) {
    uint32_t a =  (x << 2) | (x >> 30);
    return a ^ 0xf84f;
}
uint32_t p6_ops_4(uint32_t x) {
    return ((x >> 1) ^ x) + 0x5257;
}
uint32_t p6_ops_5(uint32_t x) {
    return (x >> 5) | (x << 27);
}

uint32_t do_call(uint32_t x, uint32_t choice) {
    switch (choice) {
        case 0: return p6_ops_0(x);
        case 1: return p6_ops_4(x);
        case 2: return p6_ops_3(x);
        case 3: return p6_ops_5(x);
        case 4: return p6_ops_4(x);
        case 5: return p6_ops_1(x);
        case 6: return p6_ops_2(x);
        case 7: return p6_ops_1(x);
        default: return x; // This case should never happen, but we set a default value to avoid uninitialized variable warning.
    }
}

int solve_phase_6(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;
    (void)challenge;
    // TODO: 根据 phase_06 的汇编，用 challenge 算出口令中的各个整数，依次存入 values，并把 count 改为整数个数。
    uint32_t x = (uint32_t)(challenge >> 6) ^ (challenge >> 32);
    uint32_t choice_1 = (uint32_t)(challenge & 0x7);
    uint32_t choice_2 = (uint32_t)(challenge >> 3) & 0x7;
    uint32_t ans_1 = do_call(x, choice_1);
    uint32_t ans_2 = do_call(ans_1 ^ 0x5e66, choice_2);
    values[0] = ans_2 + 0x6618;
    count = 1;
    return write_answer(out, cap, values, count);
}

int solve_secret(uint64_t challenge, uint64_t context, char *out, size_t cap) {
    uint32_t a = (uint32_t)context & 3u;
    uint32_t challenge_lowXorHigh = (uint32_t)(challenge >> 32) ^ (uint32_t)challenge;
    uint32_t context_lowXorHigh = (uint32_t)(context >> 32) ^ (uint32_t)context;
    uint32_t r9d = a * 0x3081u ^ (uint32_t)(context >> 16);

    uint32_t input = challenge_lowXorHigh ^ context_lowXorHigh;
    uint32_t mix = ((input >> 5) ^ 0x2debu) + (input ^ 0x3081u);
    uint32_t b = mix ^ r9d;
    uint32_t c = b ^ (context_lowXorHigh + a + 0x2debu +
                      (challenge_lowXorHigh >> 5));

    if (cap <= 32) return 1;
    memset(out, 'A', 32);
    int written = snprintf(out + 32, cap - 32, "OPEN%" PRIu32 " %" PRIu32 " %" PRIu32, a, b, c);
    return written < 0 || (size_t)written >= cap - 32;
}
