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

struct treeNode
{
    int data;
    struct treeNode* leftChild;
    struct treeNode* rightChild;
};

int fun7(struct treeNode* p, int v)
{
    if (p == NULL)
        return -1;
    else if (v < p->data)
        return 2 * fun7(p->leftChild, v);
    else if (v == p->data)
        return 0;
    else 
        return 2 * fun7(p->rightChild, v) + 1;
}

int main() {
	for (int i = 1001; i >= 1; --i);
	return 0;
}