#ifndef _VECTOR_H
#define _VECTOR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct vector_ {
	size_t tam;
	size_t last_used;
	long *datos;
} vector_t;

/*******************************************************************
 *                    VECTOR FUNCTIONS
 ******************************************************************/

// Crea un vector de tamaño tam
// Post: vector es una vector vacío de tamaño tam
vector_t* vector_create();

// Destruye el vector
// Pre: el vector fue creado
// Post: se eliminaron todos los elementos del vector
void vector_destroy(vector_t *vector);


// Almacena en valor el dato guardado en la posición pos del vector
// Pre: el vector fue creado
// Post: se almacenó en valor el dato en la posición pos. Devuelve false si la
// posición es inválida (fuera del rango del vector, que va de 0 a tamaño-1)
bool vector_get(vector_t *vector, size_t pos, long *valor);

// Almacena el valor en la posición pos
// Pre: el vector fue creado
// Post: se almacenó el valor en la posición pos. Devuelve false si la posición
// es inválida (fuera del rango del vector, que va de 0 a tamaño-1) y true si
// se guardó el valor con éxito.
bool vector_set(vector_t *vector, size_t pos, long valor);

// Devuelve el tamaño del vector
// Pre: el vector fue creado
size_t vector_get_size(vector_t *vector);


#endif // _VECTOR_H
