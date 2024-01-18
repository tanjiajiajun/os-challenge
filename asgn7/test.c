#include <stdio.h>
#include <inttypes.h>
#define SQUARE(x) (x*x)
void func(int x++, int --y){
printf("x = %d\ny= %d\n", x, y);
}

int main(void){
    /*char *s = "Bye";
    goodbye
    goodbye
    printf("%lu", sizeof(s));
    printf("%s", s);

    int x = 5;
    do{
        x-=2;
    } while(x>=0);
    printf("%d\n", x);
    printf("%d\n", SQUARE(3+5));
    uint64_t *x = (uint64_t *) 0x1000;
    printf("%p %p\n", (void *)x, (void *)(x+1));

    char s[] = {'r', 'a', 'c', 'e', 'c', 'a', 'r', '\0'};
    char *t = s;
    s[4] = '\0';
    printf("%s\n", t);
    */
int x =5;
int y = 5;
func(x, y);




}
