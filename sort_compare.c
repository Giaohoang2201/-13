/*
 * 과제 13: 병합 정렬과 퀵 정렬의 성능 비교
 *  - Merge Sort
 *  - Quick Sort (First / Random / Median-of-Three)
 *  - 측정 항목: 숫자 비교 횟수, 실행시간(ms), 추가 메모리 사용량(Byte)
 *
 * 컴파일: gcc -O2 -o sort_compare sort_compare.c
 * 실행  : ./sort_compare
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ------------------------------------------------------------------
 * 메모리 계산 기준 (64비트 환경 가정)
 *  - 재귀 호출 1회당 스택 프레임 크기를 아래 상수로 계산한다.
 *    매개변수 + 지역변수 + 복귀 주소(8) + 저장된 rbp(8) 를 16바이트 정렬.
 *    merge_sort(int*a, int*tmp, int lo, int hi) : 8+8+4+4 + mid(4) + 16 = 44 -> 48
 *    quick_sort(int*a, int lo, int hi, int mode): 8+4+4+4 + p(4)   + 16 = 40 -> 48
 *  - 최대 추가 메모리 = (병합 정렬) 임시 배열 n*sizeof(int) + 최대재귀깊이*프레임
 *                       (퀵 정렬)   최대재귀깊이*프레임
 * ------------------------------------------------------------------ */
#define MERGE_FRAME_BYTES 48
#define QUICK_FRAME_BYTES 48

enum { PIVOT_FIRST = 0, PIVOT_RANDOM = 1, PIVOT_MEDIAN3 = 2 };

static long long g_cmp = 0;     /* 숫자 비교 횟수 */
static int g_depth = 0;         /* 현재 재귀 깊이 */
static int g_max_depth = 0;     /* 최대 재귀 깊이 */
static unsigned long long g_rng = 88172645463325252ULL; /* xorshift64 상태 */

static unsigned long long next_rand(void)
{
    g_rng ^= g_rng << 13;
    g_rng ^= g_rng >> 7;
    g_rng ^= g_rng << 17;
    return g_rng;
}

static void swap_int(int *x, int *y)
{
    int t = *x;
    *x = *y;
    *y = t;
}

static void enter_call(void)
{
    g_depth++;
    if (g_depth > g_max_depth) g_max_depth = g_depth;
}

/* ===================== 병합 정렬 ===================== */
static void merge(int *a, int *tmp, int lo, int mid, int hi)
{
    int i = lo, j = mid + 1, k = lo;

    while (i <= mid && j <= hi) {
        g_cmp++;                         /* 값 비교 */
        if (a[i] <= a[j]) tmp[k++] = a[i++];
        else              tmp[k++] = a[j++];
    }
    while (i <= mid) tmp[k++] = a[i++];
    while (j <= hi)  tmp[k++] = a[j++];
    for (k = lo; k <= hi; k++) a[k] = tmp[k];
}

static void merge_sort_rec(int *a, int *tmp, int lo, int hi)
{
    enter_call();
    if (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        merge_sort_rec(a, tmp, lo, mid);
        merge_sort_rec(a, tmp, mid + 1, hi);
        merge(a, tmp, lo, mid, hi);
    }
    g_depth--;
}

/* 임시 배열 할당 포함. 반환값: 최대 추가 메모리(Byte) */
static long long merge_sort(int *a, int n)
{
    int *tmp = (int *)malloc(sizeof(int) * (size_t)n);
    if (tmp == NULL) { fprintf(stderr, "malloc 실패\n"); exit(1); }
    g_depth = g_max_depth = 0;
    merge_sort_rec(a, tmp, 0, n - 1);
    free(tmp);
    return (long long)n * (long long)sizeof(int)
         + (long long)g_max_depth * MERGE_FRAME_BYTES;
}

/* ===================== 퀵 정렬 ===================== */
/* 세 원소의 중앙값을 lo 위치로 옮긴다 (비교 3회를 모두 센다) */
static void median_of_three_to_lo(int *a, int lo, int hi)
{
    int mid = lo + (hi - lo) / 2;

    g_cmp++; if (a[lo]  > a[mid]) swap_int(&a[lo],  &a[mid]);
    g_cmp++; if (a[lo]  > a[hi])  swap_int(&a[lo],  &a[hi]);
    g_cmp++; if (a[mid] > a[hi])  swap_int(&a[mid], &a[hi]);
    /* 이제 a[lo] <= a[mid] <= a[hi], 중앙값은 a[mid] */
    swap_int(&a[lo], &a[mid]);
}

static int partition(int *a, int lo, int hi, int mode)
{
    int pivot, i, j;

    if (mode == PIVOT_RANDOM) {
        int r = lo + (int)(next_rand() % (unsigned long long)(hi - lo + 1));
        swap_int(&a[lo], &a[r]);
    } else if (mode == PIVOT_MEDIAN3) {
        median_of_three_to_lo(a, lo, hi);
    }
    /* PIVOT_FIRST: a[lo] 가 그대로 피벗 */

    pivot = a[lo];
    i = lo;
    for (j = lo + 1; j <= hi; j++) {
        g_cmp++;                         /* 값 비교 */
        if (a[j] < pivot) {
            i++;
            swap_int(&a[i], &a[j]);
        }
    }
    swap_int(&a[lo], &a[i]);
    return i;
}

static void quick_sort_rec(int *a, int lo, int hi, int mode)
{
    enter_call();
    if (lo < hi) {
        int p = partition(a, lo, hi, mode);
        quick_sort_rec(a, lo, p - 1, mode);
        quick_sort_rec(a, p + 1, hi, mode);
    }
    g_depth--;
}

/* 반환값: 최대 추가 메모리(Byte) = 최대재귀깊이 * 프레임 크기 */
static long long quick_sort(int *a, int n, int mode)
{
    g_depth = g_max_depth = 0;
    quick_sort_rec(a, 0, n - 1, mode);
    return (long long)g_max_depth * QUICK_FRAME_BYTES;
}

/* ===================== 보조 함수 ===================== */
static int is_sorted(const int *a, int n)
{
    int i;
    for (i = 1; i < n; i++)
        if (a[i - 1] > a[i]) return 0;
    return 1;
}

static double now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1.0e6;
}

typedef struct {
    const char *name;
    long long cmp;
    double ms;
    long long mem;
    int ok;
} Result;

/* algo: 0=Merge, 1=Quick First, 2=Quick Random, 3=Quick Median3 */
static Result run_algo(int algo, const int *orig, int *work, int n)
{
    static const char *names[] = {
        "Merge Sort", "Quick (First)", "Quick (Random)", "Quick (Median3)"
    };
    Result r;
    double t0, t1;

    memcpy(work, orig, sizeof(int) * (size_t)n);   /* 복사 시간은 측정 제외 */
    g_cmp = 0;

    t0 = now_ms();
    if (algo == 0) r.mem = merge_sort(work, n);
    else           r.mem = quick_sort(work, n, algo - 1);
    t1 = now_ms();

    r.name = names[algo];
    r.cmp = g_cmp;
    r.ms = t1 - t0;
    r.ok = is_sorted(work, n);
    return r;
}

int main(void)
{
    const int sizes[] = { 1000, 2000, 4000, 8000, 16000, 20000 };
    const int num_sizes = (int)(sizeof(sizes) / sizeof(sizes[0]));
    int s, a, i;

    srand(12345);                       /* 재현 가능한 데이터 */
    g_rng = 88172645463325252ULL;

    for (s = 0; s < num_sizes; s++) {
        int n = sizes[s];
        int *orig = (int *)malloc(sizeof(int) * (size_t)n);
        int *work = (int *)malloc(sizeof(int) * (size_t)n);
        int all_ok = 1;

        if (orig == NULL || work == NULL) { fprintf(stderr, "malloc 실패\n"); return 1; }

        /* 무작위 정수 (중복 허용): 0 ~ 99999 */
        for (i = 0; i < n; i++) orig[i] = rand() % 100000;

        printf("Data Size : %d\n\n", n);
        printf("%-16s%-15s%-12s%s\n", "Algorithm", "Comparisons", "Time(ms)", "Memory(Bytes)");
        printf("--------------------------------------------------------\n");

        for (a = 0; a < 4; a++) {
            Result r = run_algo(a, orig, work, n);
            printf("%-16s%-15lld%-12.4f%lld\n", r.name, r.cmp, r.ms, r.mem);
            if (!r.ok) all_ok = 0;
        }
        printf("\n[검증] 모든 정렬 결과 오름차순 확인: %s\n\n", all_ok ? "OK" : "FAIL");

        free(orig);
        free(work);
    }
    return 0;
}
