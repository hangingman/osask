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

# doc gen
%.html : %.texi
	$(MAKEINFO) --html --css-include=$(CSS_PATH) $<
