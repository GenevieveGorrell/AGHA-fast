
set path=%PATH%;C:\Program\cygwin\bin

gcc -O2 gha_lsa.c -o ..\bin\gha_lsa.exe
gcc -O2 process_wordbag_corpus.c -o ..\bin\process_wordbag_corpus.exe
gcc -O2 process_wordbag_corpus_svdlibc.c -o ..\bin\process_wordbag_corpus_svdlibc.exe
pause
