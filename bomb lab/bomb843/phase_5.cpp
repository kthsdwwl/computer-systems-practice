#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <set>
#include <stack>
#include <vector>
#include <iostream>
#include <string>
#include <algorithm>
using namespace std;

void foo(int input) {
	input &= 0xf;
	int Array[16] = {10, 2, 14, 7, 8, 12, 15, 11, 0, 4, 1, 13, 3, 9, 6, 5};
	int eax = input;
	int ecx = 0;
	int edx = 0;
	while (eax != 0xf) {
		++edx;
		eax = Array[eax];
		ecx += eax;
	}
	printf("Input is %d\n", input);
	printf("Iter number is %d\n", edx);
	printf("ECX is %d\n", ecx);
}

int main() {

	for (int i = 0; i <= 16; ++i) {
		foo(i);
	}
	return 0;
}