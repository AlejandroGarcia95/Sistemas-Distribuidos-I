#include <stdio.h>

int hola_local(void) {
	static int h;
	h++;
	return h;
}


int main(void){
	int pepito = hola_local();
	printf("%d\n", pepito);
	return 0;
}
