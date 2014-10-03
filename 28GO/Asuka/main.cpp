#include "parser.h"

void usage(void){
	puts("Asuka Ver1.1 by hideyosi");
	puts("\tUsage: Asuka <InputFile> <OutputFile>");
}

int main(int argc, char* argv[]){
	Parser	parser;
	if(argc < 2){
		usage();
	}else if(argc < 3){
		return (int) parser.Compile(string(argv[1]));
	}else{
		return (int) parser.Compile(string(argv[1]), string(argv[2]));
	}
	return 0;
}
