#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAMANIOMAXIMO 20

#define NUMERO_DE_ALUMNOS 3

struct persona {
  char nombre[TAMANIOMAXIMO];
  int edad;
  float ingresos;
};

float ingresosMedios(struct persona []);
int getMasJoven(struct persona gente[]);


int main (int argc, char *argv[]) {
  
  struct persona alumnos[NUMERO_DE_ALUMNOS]={{"Juan", 23, 777.11}, {"Luis",19,111.1}, {"Pedro",56,9327.2}};
  
  printf("La posición del alumno con menor edad es %d\n", getMasJoven(alumnos));
  printf("Los ingresos medios de los alumnnos son %f\n", ingresosMedios(alumnos));
  exit(1);
}

float ingresosMedios(struct persona gente[]) {
  
  int i;
  float acumulado=0;
    
  for (i=0;i<NUMERO_DE_ALUMNOS;i++)
    acumulado += gente[i].ingresos;
  
  return acumulado/NUMERO_DE_ALUMNOS;
}
  
  int getMasJoven(struct persona gente[]) {
	if (sizeof(gente) == 0)
		return -1;
	  
	int edadMoreJoven=-1;
	int i;
	for (i = 0; i < NUMERO_DE_ALUMNOS;i++) {
		if (i==0)
			edadMoreJoven = 0;
		
		if (gente[edadMoreJoven].edad < gente[i].edad) {
			edadMoreJoven = i;
		}
	}
	return edadMoreJoven;
  }

// 1. Compilar con "gcc 09ArrayDeEstructuras.c"
// 2. Ejecutar con "./a.out"
// 3. Añade una segunda función que muestre el nombre del alumno más joven y devuelva como resultado la posición de dicho alumno en el array de alumnos

