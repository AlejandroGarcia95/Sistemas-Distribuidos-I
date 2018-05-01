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

// Crea un vector de tama�o tam
// Post: vector es una vector vac�o de tama�o tam
vector_t* vector_create();

// Destruye el vector
// Pre: el vector fue creado
// Post: se eliminaron todos los elementos del vector
void vector_destroy(vector_t *vector);


// Almacena en valor el dato guardado en la posici�n pos del vector
// Pre: el vector fue creado
// Post: se almacen� en valor el dato en la posici�n pos. Devuelve false si la
// posici�n es inv�lida (fuera del rango del vector, que va de 0 a tama�o-1)
bool vector_get(vector_t *vector, size_t pos, long *valor);

// Almacena el valor en la posici�n pos
// Pre: el vector fue creado
// Post: se almacen� el valor en la posici�n pos. Devuelve false si la posici�n
// es inv�lida (fuera del rango del vector, que va de 0 a tama�o-1) y true si
// se guard� el valor con �xito.
bool vector_set(vector_t *vector, size_t pos, long valor);

// Devuelve el tama�o del vector
// Pre: el vector fue creado
size_t vector_get_size(vector_t *vector);


#endif // _VECTOR_H
