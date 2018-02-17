#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAMANIOMAXIMO 20

typedef struct persona {
  char nombre[TAMANIOMAXIMO];
  int edad;
  float ingresos;
} Persona;

int main (int argc, char *argv[]) {
  char* nom = (char*)malloc(TAMANIOMAXIMO * sizeof(char));
  printf("Nombre del profesor: ");
  scanf("%s", nom);
  
  int ed = 0;
  printf("Edad del profesor: ");
  scanf("%d", &ed);
  
  float ing = 0;
  printf("Ingresos del profesor: ");
  scanf("%f", &ing);
  
  
  Persona profesor={*nom, ed, ing};
  
  Persona alumno;
  
  strcpy(alumno.nombre,"Juan");	// *alumno.nombre = &("Juan");
  alumno.edad=19;
  alumno.ingresos=11.777;
  
  printf("Los ingresos conjuntos de alumnno y profesor son %f\n",alumno.ingresos+profesor.ingresos);
  exit(1);
}

// 1. Compilar con "gcc 08Estructura.c"
// 2. Ejecutar con "./a.out"
// 3. Modifica el programa para que el nombre, edad e ingresos del profesor se obtengan de línea de comandos

