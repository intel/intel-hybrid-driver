#include <stdio.h>
#include <stdlib.h>
#include "test_add.h"

class test_add_class {
public:
	int x, y;
	void test_function() {
		return;
	}
};

void test_add_function(int x, int y) {
	test_add_class data;

	data.x = x;
	data.y = y;

	data.test_function();
}
