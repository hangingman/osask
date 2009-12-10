aska init.ask init.3as
copy /A basehead.asm/A+boot.asm/A+init.3as/A+basefoot.asm/A base.3as/A
masmcnv2 -s -l base.3as base.asm
ml /c /Sa /Flbase.lst /Zm base.asm
link base,base,base,nul,nul
