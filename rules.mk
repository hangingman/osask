#
# OSASK suffix rules for Makefile
#

# 
# Automake supports an include directive that can be used to include 
# other Makefile fragments when automake is run.
# reference: http://www.gnu.org/software/automake/manual/html_node/Include.html 
# 
# Usage  : include $(srcdir)/file 
# 
# Example: include $(top_builddir)/rules.mk
#

#
# Set GO Tools path, $(EXEEXT) will be automatically put by automake
#
SUFFIXES = .html .ask .ias .3as

GAS2NASK	= $(top_builddir)/20GO/toolstdc/gas2nask$(EXEEXT)
NASK		= $(top_builddir)/20GO/toolstdc/nask$(EXEEXT)
LIBRARIAN	= $(top_builddir)/20GO/toolstdc/golib00w$(EXEEXT)
ASKA		= $(top_builddir)/28GO/aska/aska$(EXEEXT)
NASKCNV		= $(top_builddir)/20GO/toolstdc/naskcnv0$(EXEEXT)

#
# For generating documents
#
CSS_PATH = $(top_builddir)/misc/google-code-prettify/prettify.css
JS_PATH  = $(top_builddir)/misc/google-code-prettify/prettify.js

# doc gen
%.html : %.texi
	$(MAKEINFO) --html --css-include=$(CSS_PATH) $< --output=$@
	@echo $(CURDIR)
	@for file in `find $@ -name \*.html -type f`; do                                    \
	   echo "Replace css info at $${file}...";                                 \
	   $(SED) -i 's/<pre class="example"/<pre class="prettyprint"/g' $${file}; \
	   $(SED) -i 's/<\/head>/<script type=\"text\/javascript\" src=\"prettify.js\"><\/script>\n<\/head>/g' $${file}; \
	   $(SED) -i 's/<body /<body onload=\"prettyPrint()\" /g' $${file}; \
	done
	@echo "Copying prettify.js..."
	cp -f $(JS_PATH) $@
