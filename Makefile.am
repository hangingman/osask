SUBDIRS = 20GO

# ChangeLog
dist-hook: changelog

# Update when configure.ac is changed
changelog: configure.ac
	git log --stat --name-only --date=short --abbrev-commit > ChangeLog
