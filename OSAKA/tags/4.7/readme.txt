OSAKA src Ver4.7
                                                                    by hideyosi

１．これは何か？

  "OSAKA src Ver4.7"は、OSAKA Ver4.7の「OSAKA.EXE」を生成するのに必要な
  モジュールのソースプログラムのセットです。

２．makeの方法

  まず、hidetol_8を準備します。
  hidetol_8内のz_toolsディレクトリをこのソースのディレクトリの横に置いてくださ
  い。
  
　こんな感じね。
　
　お好きなディレクトリ
　　　　│
　　　　├─ osakasrc47
　　　　│　　　　├ readme.txt　　　（このファイル）
　　　　│　　　　├ Makefile
　　　　│　　　　　　　・
　　　　│　　　　　　　・
　　　　├─ z_tools
　　　　│　　　　├ 28GOcc1.exe
　　　　　　　　　　　　・
　　　　　　　　　　　　・

　あとは、osakasrc47に降りて、!cons_nt.batをダブルクリックしてください。
　コマンドプロンプトが開き、自動的にosakasrc47ディレクトリに降りてくれます。
　あとは、
　
　prompt>make run
　
　で、コンパイルして、qemuで動作します。
　

４．著作権とライセンス

　OSAKAの著作権はhideyosiにあります。川合秀実氏ではないことに注意してください。
　このアーカイブ内の全てのファイルに対してKL-01を適用します。
