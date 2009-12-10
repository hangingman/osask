@echo off
lcc -EP -DPCAT init.ask
rem lcc -EP -DTOWNS init.ask
aska init.i init.3as
copy /A basehead.asm/A+boot.asm/A+init.3as/A+basefoot.asm/A base.3as/A > nul
masmcnv2 -s -l base.3as base.asm
ml /c /Sa /Flbase.lst /Zm base.asm
link base,base,base,nul,nul
