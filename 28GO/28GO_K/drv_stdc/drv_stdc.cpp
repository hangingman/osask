#include <go_stdio.hpp>  /* fopen, fclose, fread, fwrite, fseek, ftell, NULL */
#include <go_stdlib.hpp> /* exit */

extern GO_FILE GO_stdin, GO_stdout, GO_stderr;

#include "go_lib.hpp"
#include "others.hpp"
#include "wfile_t.cpp"
#include "../funcs/cc1sub.cpp"

void GOL_sysabort(UCHAR termcode)
{
	static const char *termmsg[] = {
		"",
		"[TERM_WORKOVER]\n",
		"[TERM_OUTOVER]\n",
		"[TERM_ERROVER]\n",
		"[TERM_BUGTRAP]\n",
		"[TERM_SYSRESOVER]\n",
		"[TERM_ABORT]\n"
	};
	GO_stderr.p1 += 128; /* 予備に取っておいた分を復活 */
	/* バッファを出力 */
	if (GO_stdout.p - GO_stdout.p0) {
		if (GOLD_write_t(GOL_outname, GO_stdout.p - GO_stdout.p0, GO_stdout.p0)) {
			GO_fputs("GOL_sysabort:output error!\n", &GO_stderr);
			termcode = 6;
		}
	}
	if (termcode <= 6)
		GO_fputs(termmsg[termcode], &GO_stderr);
	if (GO_stderr.p - GO_stderr.p0)
		GOLD_write_t(NULL, GO_stderr.p - GO_stderr.p0, GO_stderr.p0);
	GOLD_exit((termcode == 0) ? GOL_retcode : 1);
}
