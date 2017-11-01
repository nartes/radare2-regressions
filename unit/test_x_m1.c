#include "minunit.h"
#include <experimental/x_m1.h>
#include <experimental/x_m1_private.h>
#include <stdlib.h>

bool test_r_sort1_asc (void) {
	int n = 10;
	RBinXS1 *b = R_NEWS (RBinXS1, 2 * n);
	int i;

	for (i = 0; i < n; ++i) {
		b[2 * i].off = rand () % 100;
		b[2 * i].start = true;
		b[2 * i].s_id = i;

		b[2 * i + 1].off = b[2 * i].off + rand () % 100;
		b[2 * i + 1].start = false;
		b[2 * i + 1].s_id = i;
	}

	r_bin_x_sort1_asc (b, b + 2 * n, sizeof (RBinXS1), (RBinXComp)r_bin_x_cmp2);

	for (i = 1; i < 2 * n; ++i) {
		mu_assert_eq (true, r_bin_x_cmp2 (&b[i - 1], &b[i]) < 0, "ascending sorting");
	}

	R_FREE (b);

	mu_end;
}

//    Sections vs Segments test #2
//    #1               #2            #3
// 1 ...... 10 20 .......... 50 60 ............. 90
//             #4           #5                  #6
//      5.............30 41 .........65 66 ............100
//            #7           #8     #9            #10
//    3...............30 41..42 43...65  67 ...........99
//          #11                #12             #13
//      5.............30 41 .........65 66 ............100
//          #14            #15    #16          #17
//    3...............30 41..42 43...65  67 ...........99
//  #18
//  2..3
//
// [1,#1 [2,#18 [3,#7 [3,#14 ]3,#18 [5,#4 [5,#11, ]10,#1 [20,#2 ]30,#14 ]30,#11 ]30,#7 ]30,#4
// [41,#5 [41,#8 [41,#12 [41,#15 ]42,#15 ]42,#8 [43,#9 [43,#16 ]50,#2 [60,#3
// ]65,#16 ]65,#12 ]65,#9 ]65,#5 [66,#6 [66,#13 [67,#10 [67,#17 ]90,#3
// ]99,#17 ]99,#10 ]100,#13 ]100,#6
//
// 1 1,
// 2 1, 18,
// 3 1, 7, 14,
// 5 1, 4, 7, 11, 14,
// 10 4, 7, 11, 14,
// 20 2, 4, 7, 11, 14,
// 30 2,
// 41 2, 5, 8, 12, 15,
// 42 2, 5, 12,
// 43 2, 5, 9, 12, 16,
// 50 5, 9, 12, 16,
// 60 3, 5, 9, 12, 16,
// 65 3,
// 66 3, 6, 13
// 67 3, 6, 10, 13, 17,
// 90 6, 10, 13, 17
// 99 6, 13
// 100
//
// push 1
// 1 .. 2 #1
// push 18
// 2 .. 3 #1, #18
// push 7
// push 14
// pop 18
// 3 .. 5 #1, #7, #14
// push 4
// push 11
// 5 .. 10 #1, #4, #7, #11, #14
// ...
//
// Queries:
//
// 1, 1
// 2, 1, 18
// 29 2, 4, 7, 11, 14
// 9 1, 4, 7, 11, 14
// 89 3, 6, 10, 13, 17
// 99 6, 13
//
//                 i      w x
// Merge 1 18 with 7, 14, 18

bool test_r_f2 (void) {
	int a[] = {1, 10, 20, 50, 60, 90, 5, 30, 41, 65, 66, 100,
		   3, 30, 41, 42, 43, 65, 67, 99, 5, 30, 41, 65,
		   66, 100, 3, 30, 41, 42, 43, 65, 67, 99, 2, 3, -1};

	int z[] = {
		1, 1, -1,
		2, 1, 18, -1,
		3, 1, 7, 14, -1,
		5, 1, 4, 7, 11, 14, -1,
		10, 4, 7, 11, 14, -1,
		20, 2, 4, 7, 11, 14, -1,
		30, 2, -1,
		41, 2, 5, 8, 12, 15, -1,
		42, 2, 5, 12, -1,
		43, 2, 5, 9, 12, 16, -1,
		50, 5, 9, 12, 16, -1,
		60, 3, 5, 9, 12, 16, -1,
		65, 3, -1,
		66, 3, 6, 13, -1,
		67, 3, 6, 10, 13, 17, -1,
		90, 6, 10, 13, 17, -1,
		99, 6, 13, -1,
		100, -1,
		-1};

	int y[] = {
		1, 1, -1,
		2, 1, 18, -1,
		29, 2, 4, 7, 11, 14, -1,
		9, 1, 4, 7, 11, 14, -1,
		89, 3, 6, 10, 13, 17, -1,
		99, 6, 13, -1,
		-1};

	int n = 0;
	int i, m, k, j, u;
	RBinXS1 *b = NULL;
	RBinXS3 *c = NULL;
	RBinXS4 *d = NULL;

	while (a[n] >= 0)
		++n;

	mu_assert_eq (0, n % 2, "even length array");

	n /= 2;

	b = R_NEWS (RBinXS1, 2 * n);

	for (i = 0; i < n; ++i) {
		b[2 * i].off = a[2 * i];
		b[2 * i].start = true;
		b[2 * i].s_id = i;

		b[2 * i + 1].off = a[2 * i + 1];
		b[2 * i + 1].start = false;
		b[2 * i + 1].s_id = i;
	}

	r_bin_x_sort1_asc (b, b + 2 * n, sizeof (RBinXS1), (RBinXComp)r_bin_x_cmp2);

	c = NULL;
	r_bin_x_f2 (b, n, &c, &m);

	for (j = 0, k = 0; z[j] != -1; ++k, ++j) {
		mu_assert_eq (z[j], c[k].off, "same event offset");
		++j;

		for (i = 0; z[j] != -1 && i < c[k].l; ++i, ++j) {
			mu_assert_eq (z[j], c[k].s[i] + 1, "same sections per event");
		}

		mu_assert_eq (true, i == c[k].l && z[j] == -1, "same num of sections");
	}

	mu_assert_eq (m, k, "equal segment events");

	d = NULL;
	u = r_bin_x_f3 (c, m, &d);

	RBinXS5 e[1];

	e[0].d = d;
	e[0].u = u;

	for (j = 0; y[j] != -1; ++j) {
		r_bin_x_f6_bt (e, y[j], 0);

		d = r_bin_x_f8_get_all (e, 0);

		if (y[j] == -1) {
			continue;
		}


		mu_assert_eq (true,
			      d != NULL,
			      "sections are present");

		mu_assert_eq (true,
			      d->from <= y[j] && y[j] < d->to,
			      "section lies within a segment");

		++j;

		for (i = 0; y[j] != -1 && i < d->l; ++i, ++j) {
			mu_assert_eq (y[j], d->s[i] + 1, "same sections per segment");
		}

		mu_assert_eq (true, i == d->l && y[j] == -1, "same num of sections");
	}

	R_FREE (b);
	for (k = 0; k < m; ++k) {
		R_FREE (c[k].s);
	}
	R_FREE (c);

	for (k = 0; k < u; ++k) {
		R_FREE (e[0].d[k].s);
	}
	R_FREE (e[0].d);

	mu_end;
}

int all_tests () {
	mu_run_test (test_r_sort1_asc);
	mu_run_test (test_r_f2);

	return tests_passed != tests_run;
}

int main (int argc, char **argv) {
	return all_tests ();
}
