unsigned int GO_strlen(const char *s)
{
	const char *t = s;
	while (*s)
		s++;
	return s - t;
}
