#include <stdio.h>

int main(void){
	int input;
	int tmp[10];
	scanf("%d", &input);
	if(input == 1){
		tmp[0] = 1;
		for(int i = 0; i < 10000; i++){
			for(int j = 0; j < 10; j++){
				tmp[j] *= tmp[j] + i * j;
				tmp[i % 10] += tmp[j] + tmp[j * j % 10];
				tmp[i * j % 10] += tmp[i % 10] + tmp[j];
			}
		}	
	}

	return 0;
}
