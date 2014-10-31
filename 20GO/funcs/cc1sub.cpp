/* cc1sub.c:cc1, cpp0, cc1plus用のドライバ補助 */
/* w32, stdc共通。osaは違う */

struct GOL_STR_DIR;

typedef struct GOL_STR_FILE {
	unsigned int size;
	char *p0;
	unsigned int linkcount;
	void *p_sys;
} GOL_FILE;

int GOL_retcode = 0;
char *GOL_outname = NULL;
void *GOL_sysmalloc(unsigned int size);
int main1(int argc, char **argv);
int GO_fputs(const char *s, GO_FILE *stream);

void GOLD_eixt(int s);
int GOLD_getsize(const char *name);
int GOLD_read(const char *name, int len, char *b0);
int GOLD_write_t(const char *name, int len, const char *p0);
void GOL_sysabort(unsigned char termcode);

void GOL_callmain(int argc, char **argv)
{
	char **argv1, **p;
	p = argv1 = GOL_sysmalloc((argc + 1) * sizeof (char *));
	for (;;) {
		if ((*p = *argv) == NULL)
			break;
		if ((*argv)[0] == '-' && (*argv)[1] == 'o') {
			GOL_outname = &((*argv)[2]);
			if ((*argv)[2] == '\0') {
				if (argv[1] != NULL)
					GOL_outname = (argv++)[1];
			}
			p--;  /* delete -o */
		}
		p++;
		argv++;
	}
	GOL_retcode = main1(p - argv1, argv1);
	/* バッファを出力 */
	GOL_sysabort(0);
}

void GOL_close(GOL_FILE *gfp)
{
	if (--gfp->linkcount == 0) {
		GOL_sysfree(gfp->p0);
		GOL_sysfree(gfp);
	}
	return;
}

GOL_FILE *GOL_open(struct GOL_STR_DIR *dir, const char *name)
{
	GOL_FILE *gfp;
	int bytes;
	char *p;
	bytes = GOLD_getsize(name);
	if (bytes == -1)
		goto err;
	gfp = (struct GOL_STR_FILE *) GOL_sysmalloc(sizeof (struct GOL_STR_FILE));
	p = gfp->p0 = (char *) GOL_sysmalloc(bytes);
	gfp->linkcount = 1;
	if (GOLD_read(name, bytes, p)) {
		GOL_close(gfp);
		goto err;
	}
	gfp->size = bytes;
	return gfp;
err:
	return (GOL_FILE *) ~0;
}

char *GOL_stepdir(struct GOL_STR_DIR **dir, const char *path)
{
	return (char *) path;
}
