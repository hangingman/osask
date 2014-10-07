#include <guigui00.h>

void lib_steppath0(const int opt, const int slot, const char *name, const int sig)
{
	static struct {
		unsigned int cmd_name, len;
		unsigned char modulename[12];
		unsigned int cmd_sig, sig_len, sig_head, signal;
		unsigned int cmd_eosc;
	} subcommnand = {
		0xffffff03, 12, { 0 }, 0xffffff02, 2, 0x7f000001, 0, 0
	};
	int i;
	for (i = 0; i < 12; i++)
		subcommnand.modulename[i] = name[i];
	subcommnand.signal = sig;
	lib_execcmd0(0x00ac, opt, slot, sizeof subcommnand, &subcommnand, 0x000c, 0x0000);
	return;
}
