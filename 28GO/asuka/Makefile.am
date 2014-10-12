AUTOMAKE_OPTIONS        = color-tests subdir-objects

#
# binary & sources
#
ASKA		 = $(top_builddir)/28GO/asuka/asuka
bin_PROGRAMS     = asuka
asuka_SOURCES    = filepath.cpp   \
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
# documents
#
#info_TEXINFOS		= asuka.texi
html_docs               = asuka.html

#
# suffix rules
#
SUFFIXES = .ask .html
.PHONY: html

.ask:
	$(ASKA) $*.ask $*.o
.ask.o:
	$(ASKA) $< $@
.texi.html:
	$(MAKEINFO) --html $<
html: $(html_docs)





