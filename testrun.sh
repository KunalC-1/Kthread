echo "\n\t\033[1mRunning one-one Tests\n\t\033[0m"
./test/one-one/tests -v
./test/one-one/matrixmulti
./test/one-one/mergeSort

echo "\n\t\033[1mRunning many-one Tests\n\t\033[0m"
./test/many-one/tests -v
./test/many-one/matrixmulti
./test/many-one/mergeSort

echo "\n\t\033[1mRunning many-many Tests\n\t\033[0m"
./test/many-many/tests -v
./test/many-many/matrixmulti
./test/many-many/mergeSort
