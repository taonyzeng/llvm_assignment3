int dcebench2(int a, int b) {
	int c = 2 + b;
	int z = 2 * c;
  	int x = z - 2;

	for(int i = 0; i < 100; i++) {
		x = c + i;
		if (b < 0)
			c = c + 1;
		else 
			c = c + a;
		
		z = x + c;
	}
	
	return c + a;
}

int main(int argc, char const *argv[]) {
	dcebench2(2, 3);
	return 0;
}