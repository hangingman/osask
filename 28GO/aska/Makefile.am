AUTOMAKE_OPTIONS        = color-tests subdir-objects

# include rules
include $(top_builddir)/rules.mk

#
# binary & sources
#
bin_PROGRAMS     = aska
aska_SOURCES     = filepath.cpp   \
	           filepath.hpp	  \
		   generator.cpp  \
		   generator.hpp  \
		   macro.hpp	  \
		   main.cpp	  \
		   module.cpp	  \
		   module.hpp	  \
		   parser.cpp	  \
		   parser.hpp	  \
		   scanner.cpp	  \
		   scanner.hpp	  \
		   stringx.cpp	  \
		   stringx.hpp	  \
		   table.cpp	  \
		   table.hpp	  \
		   textmodule.cpp \
		   textmodule.hpp \
		   tokendef.hpp	  

#
# tests & sources
#
check_PROGRAMS          = asuka-test1
TESTS			= asuka-test1
asuka_test1_SOURCES	= asuka-test1.ask

#
# documents output directory
#
html_docs               = aska-doc.html

#
# suffix rules
#
doc: $(html_docs)

# doc clean
clean-local:
	rm -rf $(html_docs)
# 
# TODO: why this rule can't include hmm...
#
.ask.ias:
	$(CPP)  - < $*.ask > $*.ias
.ias.3as:
	$(ASKA) $*.ias $*.3as
.3as.nas:
	$(NASKCNV) $*.3as $*.nas
.nas.o:
	$(NASK) $*.nas $*.o
