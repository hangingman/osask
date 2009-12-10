// "txt2bin.c"      copyright(C) 2000 H.Kawai(êÏçáèGé¿)

#include <stdio.h>

const int main()
{
	FILE *fp0, *fp1;
	fp0 = fopen("FONTTEXT.TXT", "r");
	fp1 = fopen("ANKFONT0.BIN", "wb");
	do {
		char s[12];
		int i;
		if (fgets(s, 12, fp0) != NULL && (s[0] == ' ' || s[0] == '*')) {
			i  = (s[0] == '*') << 7;
			i |= (s[1] == '*') << 6;
			i |= (s[2] == '*') << 5;
			i |= (s[3] == '*') << 4;
			i |= (s[4] == '*') << 3;
			i |= (s[5] == '*') << 2;
			i |= (s[6] == '*') << 1;
			i |= (s[7] == '*')     ;
			fputc(i, fp1);
		}
	} while (!feof(fp0));
	fclose(fp0);
	fclose(fp1);
	return 0;
}
