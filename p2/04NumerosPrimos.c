#include <stdio.h>
#include <stdlib.h>

int main (int argc, char *argv[]) {

  int numerosPrimos[]={1,2,3,5,7,11,13,17,19,23,29};
  int posicion;

  if (argc==1)
    printf("La lista contiene los %d primeros números primos\n",(int)(sizeof(numerosPrimos)/sizeof(int)));
  else {
    posicion=atoi(argv[1]);
    printf("El número primo que ocupa la posición %d es %d\n",posicion, numerosPrimos[posicion]);
    int suma = 0;
    for (int i = 0; i<sizeof(numerosPrimos);i++) {
    	if (posicion!=i) {
		suma+=numerosPrimos[i];		
		}	
    }
    printf("La suma total de los numeros primos, a excepción del de la posición %d, es %d\n", posicion, suma);
  }
  exit(1);
}


// 1. Compilar con "gcc 04NumerosPrimos.c"
// 2. Ejecutar con "./a.out"
// 3. Obtener la ayuda sobre atoi: "man atoi"
// 4. Modificar el programa para que calcule y muestre la suma de todos los números primos de la lista excepto el indicado en línea de comandos
