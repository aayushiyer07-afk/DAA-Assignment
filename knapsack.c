/* 0/1 Knapsack (DP) with live ASCII table visualization.
 *
 * Compile:  gcc knapsack.c -o knapsack
 * Run:      ./knapsack input.txt [delay_ms] [novis]
 *   e.g.    ./knapsack input.txt 50
 *           ./knapsack input.txt 0 novis
 *
 * Input: test cases separated by lines starting with #Test
 *   #Test
 *   n W
 *   w1 v1
 *   ...
 * Output: result.txt with lines like:  #Test 1 (4 10) 90
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CLEAR "\033[H\033[J"
#define REV   "\033[7m"
#define RST   "\033[0m"
#define GRN   "\033[92m"

static int delay_ms = 50;
static int visualize = 1;

static void render(int *dp, int *wt, int *val, int n, int W,
                   int ci, int cj, int t, const char *note)
{
    int maxv = 0;
    for (int r = 0; r <= n; r++)
        for (int c = 0; c <= W; c++)
            if (dp[r * (W + 1) + c] > maxv) maxv = dp[r * (W + 1) + c];
    int cw = snprintf(NULL, 0, "%d", maxv) + 1;
    if (cw < 3) cw = 3;

    printf(CLEAR "Test %d: n=%d, W=%d\n", t, n, W);
    printf("      item(w,v) | ");
    for (int c = 0; c <= W; c++) printf("%*d", cw, c);
    printf("\n");
    for (int k = 0; k < 16 + (W + 1) * cw; k++) putchar('-');
    printf("\n");

    for (int r = 0; r <= n; r++) {
        char name[32];
        if (r == 0) strcpy(name, "none");
        else snprintf(name, sizeof name, "(%d,%d)", wt[r - 1], val[r - 1]);
        printf("  i=%-2d %9s | ", r, name);
        for (int c = 0; c <= W; c++) {
            int done = (r < ci) || (r == ci && c <= cj);
            char buf[32];
            if (done) snprintf(buf, sizeof buf, "%d", dp[r * (W + 1) + c]);
            else      strcpy(buf, ".");
            if (r == ci && c == cj) printf(REV "%*s" RST, cw, buf);
            else                    printf("%*s", cw, buf);
        }
        printf("\n");
    }
    if (note && *note) printf("\n%s\n", note);
    fflush(stdout);
}

static void pause_ms(int ms) { if (ms > 0) usleep(ms * 1000); }

static int knapsack(int n, int W, int *wt, int *val, int t)
{
    int cols = W + 1;
    int *dp = calloc((size_t)(n + 1) * cols, sizeof(int));
    int cell_mode = (W <= 20); /* wide tables: redraw once per row */
    char note[400];

    for (int i = 1; i <= n; i++) {
        for (int j = 0; j <= W; j++) {
            dp[i * cols + j] = dp[(i - 1) * cols + j];
            snprintf(note, sizeof note,
                     "dp[%d][%d] = dp[%d][%d] = %d  (skip item %d)",
                     i, j, i - 1, j, dp[i * cols + j], i);
            if (wt[i - 1] <= j) {
                int cand = dp[(i - 1) * cols + (j - wt[i - 1])] + val[i - 1];
                if (cand > dp[i * cols + j]) {
                    dp[i * cols + j] = cand;
                    snprintf(note, sizeof note,
                             "dp[%d][%d] = dp[%d][%d] + %d = %d  (take item %d)",
                             i, j, i - 1, j - wt[i - 1], val[i - 1], cand, i);
                }
            }
            if (visualize && cell_mode) {
                render(dp, wt, val, n, W, i, j, t, note);
                pause_ms(delay_ms);
            }
        }
        if (visualize && !cell_mode) {
            snprintf(note, sizeof note, "Row %d filled", i);
            render(dp, wt, val, n, W, i, W, t, note);
            pause_ms(delay_ms * 4);
        }
    }

    int best = dp[n * cols + W];

    if (visualize) {
        /* traceback: which items were chosen */
        char chosen[256] = "";
        int j = W;
        int picks[1024], cnt = 0;
        for (int i = n; i >= 1; i--)
            if (dp[i * cols + j] != dp[(i - 1) * cols + j]) {
                picks[cnt++] = i;
                j -= wt[i - 1];
            }
        for (int k = cnt - 1; k >= 0; k--) {
            char tmp[16];
            snprintf(tmp, sizeof tmp, "%d ", picks[k]);
            strncat(chosen, tmp, sizeof chosen - strlen(chosen) - 1);
        }
        snprintf(note, sizeof note, GRN "Max value = %d   items chosen = [ %s]" RST,
                 best, chosen);
        render(dp, wt, val, n, W, n, W, t, note);
        pause_ms(delay_ms * 10);
    }
    free(dp);
    return best;
}

int main(int argc, char *argv[])
{
    const char *path = (argc > 1) ? argv[1] : "input.txt";
    if (argc > 2) delay_ms = atoi(argv[2]);
    if (argc > 3 && strcmp(argv[3], "novis") == 0) visualize = 0;

    FILE *in = fopen(path, "r");
    if (!in) { perror("input file"); return 1; }
    FILE *out = fopen("result.txt", "w");
    if (!out) { perror("result.txt"); return 1; }

    char tok[64];
    int t = 0;
    while (fscanf(in, "%63s", tok) == 1) {
        if (strncmp(tok, "#Test", 5) != 0) continue;
        int n, W;
        if (fscanf(in, "%d %d", &n, &W) != 2) break;
        int *wt = malloc(n * sizeof(int));
        int *val = malloc(n * sizeof(int));
        for (int i = 0; i < n; i++)
            if (fscanf(in, "%d %d", &wt[i], &val[i]) != 2) { n = i; break; }
        t++;
        int best = knapsack(n, W, wt, val, t);
        fprintf(out, "#Test %d (%d %d) %d\n", t, n, W, best);
        printf("#Test %d (%d %d) %d\n", t, n, W, best);
        free(wt);
        free(val);
    }
    fclose(in);
    fclose(out);
    printf("\nSaved to result.txt\n");
    return 0;
}