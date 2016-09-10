gcc -O0 -std=c11 -o test_loader test_loader.c ../loader.c -lpng
gcc -O0 -std=c11 -o test_algorithms test_algorithms.c ../loader.c ../algorithms.c -lpng
gcc -O0 -std=c11 -o test_np test_np.c ../loader.c ../algorithms.c -lpng
