/*	$NetBSD: t_round.c,v 1.1 2011/01/02 03:51:21 pgoyette Exp $	*/

/*
 * Written by J.T. Conklin, Apr 18, 1995
 * Public domain.
 */

#include <atf-c.h>
#include <atf-c/config.h>

#include <ieeefp.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>

ATF_TC(fpround);

ATF_TC_HEAD(fpround, tc)
{

	atf_tc_set_md_var(tc, "descr",
	    "Minimal testing of fpgetround(3) and fpsetround(3)");
}

ATF_TC_BODY(fpround, tc)
{

	/*
	 * This test would be better if it actually performed some
	 * calculations to verify the selected rounding mode.  But
	 * this is probably acceptable since the fp{get,set}round
	 * functions usually just get or set the processors fpu
	 * control word.
	 */

	ATF_CHECK_EQ(fpgetround(), FP_RN);
	ATF_CHECK_EQ(FLT_ROUNDS, 1);

	/*
	 * At least on one port (amd64), fpsetround() doesn't have any visible
	 * effect.  So disable checking for the non-default modes for now.
	 * See PR port-amd64/44293
	 */
#ifdef NOTYET
	ATF_CHECK_EQ(fpsetround(FP_RP), FP_RN);
	ATF_CHECK_EQ(fpgetround(), FP_RP);
	ATF_CHECK_EQ(FLT_ROUNDS, 2);

	ATF_CHECK_EQ(fpsetround(FP_RM), FP_RP);
	ATF_CHECK_EQ(fpgetround(), FP_RM);
	ATF_CHECK_EQ(FLT_ROUNDS, 3);

	ATF_CHECK_EQ(fpsetround(FP_RZ), FP_RM);
	ATF_CHECK_EQ(fpgetround(), FP_RZ);
	ATF_CHECK_EQ(FLT_ROUNDS, 0);

	ATF_CHECK_EQ(fpsetround(FP_RN), FP_RZ);
#else /* NOTYET */
	ATF_CHECK_EQ(fpsetround(FP_RN), FP_RN);
#endif /* NOTYET */

	ATF_CHECK_EQ(fpgetround(), FP_RN);
	ATF_CHECK_EQ(FLT_ROUNDS, 1);
}

ATF_TP_ADD_TCS(tp)
{
	const char *arch;

	arch = atf_config_get("atf_machine");
	if (strcmp("vax", arch) == 0 || strcmp("m68000", arch) == 0)
		atf_tc_skip("Test not applicable on %s", arch);
	else {
		ATF_TP_ADD_TC(tp, fpround);
	}

	return atf_no_error();
}