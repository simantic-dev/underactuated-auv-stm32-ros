/* Added for the GCC build: the Keil project retargets printf by overriding fputc(); newlib calls _write(). */
#include <stdio.h>
#include <sys/types.h>
extern int fputc(int ch, FILE *f);
int _write(int fd, char *buf, int len) { (void)fd; for (int i = 0; i < len; i++) fputc(buf[i], NULL); return len; }
void *_sbrk(int incr) { extern char end; static char *h; char *p; if (!h) h = &end; p = h; h += incr; return p; }
int _close(int f){(void)f;return -1;} int _fstat(int f,void*s){(void)f;(void)s;return 0;} int _isatty(int f){(void)f;return 1;}
int _lseek(int f,int o,int w){(void)f;(void)o;(void)w;return 0;} int _read(int f,char*b,int l){(void)f;(void)b;(void)l;return 0;}
int _getpid(void){return 1;} int _kill(int p,int s){(void)p;(void)s;return -1;} void _exit(int s){(void)s;for(;;);}
