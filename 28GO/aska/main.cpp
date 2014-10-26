#include "parser.hpp"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

const std::string version  = std::string("aska ver1.1 for " PACKAGE_STRING);
const std::string usagestr = "\tUsage: aska <InputFile> <OutputFile>";

void usage(void)
{
     std::cout << version  << std::endl;
     std::cout << usagestr << std::endl;
}

int main(int argc, char* argv[])
{
     Parser parser;
     if(argc < 2)
     {
	  usage();
     } 
     else if(argc < 3)
     {
	  return (int) parser.Compile(std::string(argv[1]));
     }
     else
     {
	  return (int) parser.Compile(std::string(argv[1]), std::string(argv[2]));
     }

     return 0;
}
