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

# TODO: Set command relative path
#
# GAS2NASK	= gas2nask
# NASK		= nask
# LIBRARIAN	= golib00w
# ASKA		= aska
# NASKCNV	= naskcnv0

# gas  -> nask
%.nas : %.s
	$(GAS2NASK) -a $*.s $*.nas

# nask -> obj
%.o : %.nas
	$(NASK) $*.nas $*.o

# ask  -> ias ( preprocessed )
%.ias : %.ask Makefile
	$(CPP) -o $*.ias $*.ask

# ask -> asm ( i386 asm )
%.3as : %.ias Makefile
	$(ASKA) $*.ias $*.3as

# asm -> nask
%.nas : %.3as Makefile
	$(NASKCNV) $*.3as $*.nas

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
