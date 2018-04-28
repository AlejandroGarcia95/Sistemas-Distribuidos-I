//hash.c
// Alumnos: Fresia Juan Manuel (Padron: 96632)
// 	        Garcia Alejandro Martin (Padron: 96661)

#ifndef HASH_H
#define HASH_H

#include <stdbool.h>
#include <stddef.h>

/* ******************************************************************
 *                DEFINICION DE LOS TIPOS DE DATOS
 * *****************************************************************/

/* Vamos a hacer un hash abierto que va a estar re piola (?. */

typedef struct hash hash_t;

typedef struct hash_iter hash_iter_t;

typedef void (*hash_destruir_dato_f)(void *); // Me di el gusto de cambiar _t por _f

// Primitivas del hash

// Crea un hash cuyos elementos a almacenar se destruiran con
// la funcion destrur_dato. Devuelve NULL si no pudo crearlo.
// Pre: destruir_dato es una funcion valida.
// Post: el hash se creo.
hash_t *hash_crear(hash_destruir_dato_f destruir_dato);

// Guarda un par clave-dato en el hash. Devuelve true si pudo
// guardarlos con exito o false en caso contrario. Si la clave
// ya existia se sobrescribe dato.
// Pre: el hash fue creado.
// Post: el par clave-dato se guardo en la posicion correspondiente.
bool hash_guardar(hash_t *hash, const char *clave, void *dato);

// Borra el par clave-dato del hash. Devuelve el dato correspondiente
// a la clave insertada. Si la clave es invalida devuelve NULL. 
// Pre: el hash fue creado.
// Post: se elimino el par clave-dato y se devolvio el dato o NULL.
void *hash_borrar(hash_t *hash, const char *clave);

// Obtiene el valor correspondiente a la clave ingresada. Si la
// clave es invalida devuelve NULL.
// Pre: el hash fue creado.
// Post: se devuelve el dato correspondiente.
void *hash_obtener(const hash_t *hash, const char *clave);

// Verifica si la clave recibida existe en el hash. Devuelve
// true si es asi o false en caso contrario.
// Pre: el hash fue creado.
// Post: se devuelve true si la clave existe en el hash o false
// en caso contrario.
bool hash_pertenece(const hash_t *hash, const char *clave);

// Devuelve la cantidad de elementos almacenados en el hash.
// Pre: el hash fue creado.
size_t hash_cantidad(const hash_t *hash);

// Destruye el hash y todos sus elementos.
// Pre: el hash fue creado.
void hash_destruir(hash_t *hash);


/*---------------------------
    Primitivas de iterador
----------------------------*/

// Crea un iterador apuntando al hash. Devuelve NULL
// si no pudo crearse.
// Pre: el hash fue creado.
// Post: se devuelve el iterador aputando al primer
// elemento del hash o a NULL si el hash esta vacio.
hash_iter_t *hash_iter_crear(hash_t *hash);

// Mueve el iterador a la siguiente posicion del hash.
// Pre: el iterador fue creado.
// Post: el iterador apunta al elemento siguiente de la lista.
bool hash_iter_avanzar(hash_iter_t *iter);

// Obtiene el dato del elemento apuntado por el iterador.
// Pre: el iterador fue creado.
// Post: se devolvio el valor del elemento apuntado por el iterador.
const char *hash_iter_ver_actual(const hash_iter_t *iter);

// Devuelve verdadero o falso, segun el iterador haya 
// alcanzado o no el final del hash.
// Pre: el iterador fue creado.
bool hash_iter_al_final(const hash_iter_t *iter);

// Destruye el iterador. 
// Pre: el iterador fue creado.
// Post: se destruyo el iterador.
void hash_iter_destruir(hash_iter_t* iter);

#endif // HASH_H
