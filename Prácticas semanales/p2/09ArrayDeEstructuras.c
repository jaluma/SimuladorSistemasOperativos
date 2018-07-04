#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define TAMANIOMAXIMO 20

#define NUMERO_DE_ALUMNOS 3

typedef struct persona {
  char nombre[TAMANIOMAXIMO];
  int edad;
  float ingresos;
} Persona;

// deberian ir en un *.h e #include
float ingresosMedios(Persona []);
int getMasJoven(Persona gente[]);


int main (int argc, char *argv[]) {
  
  Persona alumnos[NUMERO_DE_ALUMNOS]={{"Juan", 23, 777.11}, {"Luis",19,111.1}, {"Pedro",56,9327.2}};
  
  printf("La posición del alumno con menor edad es %d\n", getMasJoven(alumnos));
  printf("Los ingresos medios de los alumnnos son %f\n", ingresosMedios(alumnos));
  exit(1);
}

float ingresosMedios(Persona gente[]) {
  
  int i;
  float acumulado=0;
    
  for (i=0;i<NUMERO_DE_ALUMNOS;i++)
    acumulado += gente[i].ingresos;
  
  return acumulado/NUMERO_DE_ALUMNOS;
}
  
  int getMasJoven(struct persona gente[]) {
	int edadMoreJoven=-1;
	int i;
	for (i = 0; i < NUMERO_DE_ALUMNOS;i++) {
		if (i==0)
			edadMoreJoven = 0;
		
		if (gente[i].edad < gente[edadMoreJoven].edad) {
			edadMoreJoven = i;
		}
	}
	return edadMoreJoven;
  }

// 1. Compilar con "gcc 09ArrayDeEstructuras.c"
// 2. Ejecutar con "./a.out"
// 3. Añade una segunda función que muestre el nombre del alumno más joven y devuelva como resultado la posición de dicho alumno en el array de alumnos

