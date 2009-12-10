masmcnv2 -s -l base.3as base.asm
ml /c /Sa /Flbase.lst /Zm base.asm
link base,base,base,nul,nul
