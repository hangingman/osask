# automake
AUTOMAKE_OPTIONS        = color-tests subdir-objects

# include rules
include $(top_builddir)/rules.mk

# task
md5:
	find . -name \*.img -print | xargs -i basename {} .img | xargs -I{} sh -c "md5sum '{}.img' > '{}.hash'"
