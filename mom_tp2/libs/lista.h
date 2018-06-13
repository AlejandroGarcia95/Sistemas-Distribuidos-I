#ifndef LISTA_H
#define LISTA_H

#include <stdbool.h>
#include <stddef.h>

/* ******************************************************************
 *                DEFINICION DE LOS TIPOS DE DATOS
 * *****************************************************************/

/* La lista está planteada como una lista de punteros genéricos. */

typedef struct lista lista_t;

typedef struct lista_iterador lista_iter_t;

/* ******************************************************************
 *                    PRIMITIVAS DE LA LISTA
 * *****************************************************************/

// Crea una lista.
// Post: devuelve una nueva lista vacía.
lista_t* lista_crear();

// Destruye la lista. Si se recibe la función destruir_dato por parámetro,
// para cada uno de los elementos de la lista llama a destruir_dato.
// Pre: la lista fue creada. destruir_dato es una función capaz de destruir
// los datos de la lista, o NULL en caso de que no se la utilice.
// Post: se eliminaron todos los elementos de la lista.
void lista_destruir(lista_t *lista, void destruir_dato(void*));

// Devuelve verdadero o falso, según si la lista tiene o no elementos.
// Pre: la lista fue creada.
bool lista_esta_vacia(const lista_t *lista);

// Agrega un nuevo elemento a la lista en la primera posicion. Devuelve falso en caso de error.
// Pre: la lista fue creada.
// Post: se agregó un nuevo elemento a la lista, dato se encuentra al principio
// de la lista.
bool lista_insertar_primero(lista_t *lista, void* dato);

// Agrega un nuevo elemento a la lista en la ultima posicion. Devuelve falso en caso de error.
// Pre: la lista fue creada.
// Post: se agregó un nuevo elemento a la lista, dato se encuentra al final
// de la lista.
bool lista_insertar_ultimo(lista_t *lista, void* dato);

// Obtiene el valor del primer elemento de la lista. Si la lista tiene
// elementos, se devuelve el valor del primero, si está vacía devuelve NULL.
// Pre: la lista fue creada.
// Post: se devolvió el primer elemento de la lista, cuando no está vacía.
void* lista_ver_primero(const lista_t *lista);


// Saca el primer elemento de la lista. Si la lista tiene elementos, se quita el
// primero de la lista, y se devuelve su valor, si está vacía, devuelve NULL.
// Pre: la lista fue creada.
// Post: se devolvió el valor del primer elemento anterior, la lista
// contiene un elemento menos, si la lista no estaba vacía.
void *lista_borrar_primero(lista_t *lista);

//Devuelve el tamaño de la lista
//Pre: la lista fue creada
//Post: se devolvio la cantidad de elementos de la lista
size_t lista_largo(const lista_t *lista);

//Crea un iterador externo apuntando a la lista
//Pre: la lista fue creada
//Post: devuelve un nuevo iterador apuntando al primer elemento de la lista
lista_iter_t *lista_iter_crear(const lista_t *lista);

//Mueve el iterador a la siguiente posicion de la lista
//Pre: el iterador fue creado
//Post: el iterador apunta al elemento siguiente de la lista
bool lista_iter_avanzar(lista_iter_t *iter);

//Obtiene el valor del elemento apuntado por el iterador
//Pre: el iterador fue creado
//Post: se devolvio el valor del elemento apuntado por el iterador
void *lista_iter_ver_actual(const lista_iter_t *iter);

//Devuelve verdadero o falso, segun el iterador haya alcanzado o no el final de la lista
//Pre: el iterador fue creado
bool lista_iter_al_final(const lista_iter_t *iter);

//Destruye el iterador 
//Pre: el iterador fue creado
//Post: se destruyo el iterador
void lista_iter_destruir(lista_iter_t *iter);

//Inserta en la lista un elemento en la posicion actual a la que apunta el iterador
//Pre: el iterador fue creado
//Post: el elemento fue insertado en la lista, en la posicion en la que apunta el iterador,
//y el elemento que antes ocupaba su lugar se encuentra en la posicion siguiente a esa
bool lista_insertar(lista_t *lista, lista_iter_t *iter, void *dato);

//Borra de la lista el elemento al cual apuntaba iter
//Pre: el iterador fue creado
//Post: el elemendo fue borrado de la lista, y el que antes era el elemento anterior a ese
//apunta al que antes era el elemento siguiente a ese
void *lista_borrar(lista_t *lista, lista_iter_t *iter);

//NO TENGO LA MENOR PUTA IDEA xd
void lista_iterar(lista_t *lista, bool (*visitar)(void *dato, void *extra), void *extra);

#endif // LISTA_H
