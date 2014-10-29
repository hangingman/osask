/* for stdc */

void msgout(const char* s)
{
	GOLD_write_t(NULL, GO_strlen(s), s);
	return;
}

void errout(const char *s)
{
	msgout(s);
	GOLD_exit(1);
}

void errout_s_NL(const char *s, const char *t)
{
	msgout(s);
	msgout(t);
	msgout(NL);
	GOLD_exit(1);
}

char* readfile(const char *name, char *b0, const char *b1)
{
	FILE *fp;
	int bytes, len = b1 - b0;
	fp = fopen(name, "rb");
	if (fp == NULL)
		errout_s_NL("can't open file: ", name);
	bytes = fread(b0, 1, len, fp);
	fclose(fp);
	if (len == bytes)
		errout("input filebuf over!" NL);
	return b0 + bytes;
}
