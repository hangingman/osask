void msgout(char *s)
{
	GOLD_write_t(NULL, GO_strlen(s), s);
	return;
}

void errout(char *s)
{
	msgout(s);
	GOLD_exit(1);
}

void errout_s_NL(char *s, char *t)
{
	msgout(s);
	msgout(t);
	msgout(NL);
	GOLD_exit(1);
}

char *readfile(char *name, char *b0, char *b1)
{
	int bytes = GOLD_getsize(name);
	if (bytes == -1)
		errout_s_NL("can't open file: ", name);
	if (b0 + bytes >= b1)
		errout("input filebuf over!" NL);
	if (GOLD_read(name, bytes, b0))
		errout_s_NL("can't read file: ", name);
	return b0 + bytes;
}
