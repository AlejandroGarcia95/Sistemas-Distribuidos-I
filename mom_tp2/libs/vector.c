#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "vector.h"

#define TAM_INIC 32
#define PASO 2

/*******************************************************************
 *                        IMPLEMENTATION
 ******************************************************************/

// Crea un vector de tama�o tam
// Post: vector es una vector vac�o de tama�o tam
vector_t* vector_create() {
	vector_t* vector = malloc(sizeof(vector_t));
	if (vector == NULL) return NULL;

	vector->datos = malloc(TAM_INIC * sizeof(long));
	if (vector->datos == NULL) {
	    free(vector);
	    return NULL;
	}
	vector->tam = TAM_INIC;
	vector->last_used = 0;
	return vector;
}

// Destruye el vector
// Pre: el vector fue creado
// Post: se eliminaron todos los elementos del vector
void vector_destroy(vector_t *vector) {
	free(vector->datos); //Debo eliminar ambos
	free(vector);
}

// Cambia el tama�o del vector
// Pre: el vector fue creado
// Post: el vector cambi� de tama�o a nuevo_tam y devuelve true
// o el vector queda intacto y devuelve false si no se pudo cambiar el tama�o
// a nuevo_tam
bool vector_resize(vector_t *vector, size_t tam_nuevo) {
	long* datos_nuevo = realloc(vector->datos, tam_nuevo * sizeof(long));
	// Cuando tam_nuevo es 0, es correcto que devuelva NULL
	// En toda otra situaci�n significa que fall� el realloc
	if (tam_nuevo > 0 && datos_nuevo == NULL) {
	    return false;
	}
	vector->datos = datos_nuevo;
	vector->tam = tam_nuevo;
	return true;
}

// Almacena en valor el dato guardado en la posici�n pos del vector
// Pre: el vector fue creado
// Post: se almacen� en valor el dato en la posici�n pos. Devuelve false si la
// posici�n es inv�lida (fuera del rango del vector, que va de 0 a tama�o-1)
bool vector_get(vector_t *vector, size_t pos, long *valor) {
	if(vector->tam <= pos)	return false;
	if(((vector->tam - 1) < pos) || (vector->tam == 0)) //Sin vector de tama�o 0
		return false;
	*valor = vector->datos[pos];
	return true;
}

// Almacena el valor en la posici�n pos
// Pre: el vector fue creado
// Post: se almacen� el valor en la posici�n pos. Devuelve false si la posici�n
// es inv�lida (fuera del rango del vector, que va de 0 a tama�o-1) y true si
// se guard� el valor con �xito.
bool vector_set(vector_t *vector, size_t pos, long valor) {
	if(((vector->tam - 1) < pos) || (vector->tam == 0))
		return false;
	if(vector->tam <= pos)
		if(!vector_resize(vector, vector->tam * PASO))
			return false;
	vector->datos[pos] = valor;
	if(pos > vector->last_used)
		vector->last_used = pos;
	return true;
}

// Devuelve el tama�o del vector
// Pre: el vector fue creado
size_t vector_get_size(vector_t *vector) {
	return vector->tam;
}
