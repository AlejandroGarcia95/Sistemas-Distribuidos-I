//hash.c
// Alumnos: Fresia Juan Manuel (Padron: 96632)
// 	        Garcia Alejandro Martin (Padron: 96661)

#include "hash.h"
#include "lista.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define LIM_FC_ARRIBA 0.75
#define LIM_FC_ABAJO 0.4
#define TAM_INIC 101
#define FAC_CONV 43

// Estructura auxiliar arreglo de primos
#define CANT_PRIMOS 13

// Proposicion: Redimensionar para arriba con FC>0.75 y para abajo con FC<0.4
const int numeros_primos[] = {101, 163, 269, 439, 719, 1171, 1951, 3253, 5419, 9029, 15053, 25087, 41813};

// Estructuras del hash:

struct hash {
	lista_t** datos;  // El hash abierto es un "array" de lista_t*
	hash_destruir_dato_f destruir_dato;
	size_t tamanio, cantidad;
};

struct hash_iter {
	size_t pos_actual;
	hash_t *hash;
	lista_iter_t *lista_iter;
};

// Estructura auxiliar par clave-valor

struct _parhash {
	void *dato;
	char *clave;
};

typedef struct _parhash par_hash_t;

// FUNCIONES AUXILIARES INTERNAS

// Busca el siguiente o el anterior numero primo a un numero primo en 
// nuestro arreglo de numeros primos.
// Pre: opcion es 'A' para el anterior o cualquier otra cosa para el siguiente.
size_t siguiente_primo(size_t numero, char opcion) {
	for(int i = 0; i<CANT_PRIMOS; i++) 
		if(numeros_primos[i] == numero)
			return opcion == 'A' ? numeros_primos[i-1] : numeros_primos[i+1];
	return numero;
}

// Funcion de hash que genera un numero a partir de una clave.
// Pre: la clave es valida.
size_t funcion_hash(const char *clave, size_t tam) {
	size_t posicion = FAC_CONV * ((int) strtol(clave, NULL, 10)), i = 1;
	while(clave[i]!='\0') {
		posicion += clave[i]*i;
		i += 1;
		}
	return posicion % tam;	
}

// Crea un par_hash con una clave y un dato. Devuelve NULL si no pudo
// crearlo.
// Pre: la clave y el dato son validos.
// Post: el par_hash se creo.
par_hash_t *par_hash_crear(const char *clave, void *dato) {
	par_hash_t *parhash = malloc(sizeof(par_hash_t));
	if(!parhash)
		return NULL;
	parhash->dato = dato;
	parhash->clave = malloc((strlen(clave)+1)*sizeof(char));
	if(!parhash->clave) {
		free(parhash);
		return NULL;
		}
	strcpy(parhash->clave, clave);
	return parhash;
}

// Busca en todo el hash el parhash que tiene como clave la clave
// recibida. Si el parhash existe devuelve un iterador apuntando a este, 
// si no devuelve NULL.
// Pre: hash fue creado, clave no es vacia.
// Post: se devolvio un iterador al parhash con esa clave o NULL.
// IMPORTANTE: la funcion pide memoria dinamicamente para el iterador.
// Si la clave no estaba el hash la funcion destruye al iterador por si
// misma, si no hay que destruirlo.
lista_iter_t *buscar_en_hash(const hash_t *hash, const char *clave) { 
	size_t posicion = funcion_hash(clave,hash->tamanio); 
	par_hash_t *parhash_actual;
	//Creo un iterador de lista para recorrer la lista de esa posicion
	lista_iter_t *iterador = lista_iter_crear(hash->datos[posicion]);
	
	// Si la lista no esta vacia, tengo que ver elemento por elemento hasta
	// que alguno coincida con la clave. 
	while(!lista_iter_al_final(iterador)) {
		parhash_actual = lista_iter_ver_actual(iterador);	
		//Si alguno coincide con la clave, devuelvo eso
		if(!strcmp(parhash_actual->clave,clave)) {
			return iterador;
			}
		lista_iter_avanzar(iterador);	
		}
	lista_iter_destruir(iterador);
	return NULL;
}

// Recibe un iterador a hash y devuelve un puntero al par_hash apuntado
// por este. Si el iter apuntaba a NULL, se devuelve eso.
// Pre: el iterador a hash fue creado.
par_hash_t *hash_iter_parhash_actual(hash_iter_t* iterador) {
	if (!iterador) 
		return NULL;
	return (par_hash_t*) (lista_iter_ver_actual(iterador->lista_iter));
}

// Crea un hash con el tamanio indicado, y cuyos elementos a almacenar
// se destruiran con la funcion destrur_dato. Devuelve NULL si no pudo 
// crearlo.
// Pre: destruir_dato es una funcion valida.
// Post: el hash se creo.
hash_t *hash_crear_con_tam(hash_destruir_dato_f destruir_dato, size_t tam) {
	hash_t *hash = malloc(sizeof(hash_t));
	if(!hash)
		return NULL;
	hash->tamanio = tam;
	hash->datos = malloc(hash->tamanio * sizeof(lista_t*));
	if(!hash->datos) {
		free(hash);
		return NULL;
		}
	// Creo todas las listas vacias
	for (int i = 0; i < hash->tamanio; i++) {
		hash->datos[i] = lista_crear();
	// Si alguna de las listas falla, limpio todo lo que cree
		if (!hash->datos[i]) {
			for (int j = 0; j <= i; j++) 
				lista_destruir(hash->datos[j],NULL);
			free(hash->datos); // Estaba faltando esto
			free(hash);
			return NULL;
			}
		}
	hash->cantidad = 0;
	hash->destruir_dato = destruir_dato;
	return hash;
}


// Decide si es necesario o no redimensionar el arreglo, de acuerdo
// a los limites del factor de carga definidos como constantes
// Pre: el hash existe (tamaño > 0)
// Post: devuelve true si es necesario redimensionar el arreglo (ya sea
// hacia arriba o hacia abajo, de lo contrario devuelve false
bool hay_que_redimensionar(hash_t *hash) {
	float fc = ((float) hash->cantidad)/((float) hash->tamanio);
	//printf("\n FACTOR DE CARGA ES %f\n", fc);
	if((fc>LIM_FC_ARRIBA) && (hash->tamanio<numeros_primos[CANT_PRIMOS-1]))
		return true; // Hay que agrandar el hash
	if((fc<LIM_FC_ABAJO) && (hash->tamanio>numeros_primos[0]))
		return true; // Hay que achicar el hash
	return false;
}


// Redimensiona un hash, aumentando o reduciendo su tamaño segun sea necesario
// Pre: el hash necesita ser redimensionado
// Post: se devuelve un hash que contiene los mismos par clave-dato que el
// hash original, pero distribuidos en un arreglo de listas con el nuevo tamaño
hash_t* redimensionar_hash(hash_t* original) {

	// Calcula el factor de carga para decidir en que direccion redimensionar
	float fc = ((float) original->cantidad)/((float) original->tamanio);
	size_t nuevo_tam = original->tamanio;
	
	// Crea un hash temporal con el nuevo tamaño
	if((fc>LIM_FC_ARRIBA) && (original->tamanio<numeros_primos[CANT_PRIMOS-1]))
		nuevo_tam = siguiente_primo(nuevo_tam, 'S'); // Primo siguiente
	else
		nuevo_tam = siguiente_primo(nuevo_tam, 'A'); // Primo anterior
	hash_t *hash_nuevo = hash_crear_con_tam(original->destruir_dato, nuevo_tam);
	par_hash_t *ph;
	
	// Itera manualmente sobre cada lista del hash original; obteniendo
	// el parhash del original, y reubicandolo en el nuevo
	for(size_t i = 0; i < original->tamanio; i++) {
		while(!lista_esta_vacia(original->datos[i])) {
			ph = lista_borrar_primero(original->datos[i]);
			lista_insertar_primero(hash_nuevo->datos[funcion_hash(ph->clave,hash_nuevo->tamanio)], ph);
			}
		lista_destruir(original->datos[i], NULL);
		}
	free(original->datos);
	// Asigno el arreglo de listas del auxiliar de nuevo al hash original
	// Y libero toda la memoria que ya no uso
	original->datos = hash_nuevo->datos;
	original->tamanio = hash_nuevo->tamanio;
	free(hash_nuevo);
	
	return original;
}

// PRIMITIVAS DEL HASH

// Crea un hash cuyos elementos a almacenar se destruiran con
// la funcion destrur_dato. Devuelve NULL si no pudo crearlo.
// Pre: destruir_dato es una funcion valida.
// Post: el hash se creo.
hash_t *hash_crear(hash_destruir_dato_f destruir_dato) {
	return hash_crear_con_tam(destruir_dato, TAM_INIC);
}

// Guarda un par clave-dato en el hash. Devuelve true si pudo
// guardarlos con exito o false en caso contrario. Si la clave
// ya existia se sobrescribe dato.
// Pre: el hash fue creado.
// Post: el par clave-dato se guardo en la posicion correspondiente.
bool hash_guardar(hash_t *hash, const char *clave, void *dato) {
	lista_iter_t *iterador = buscar_en_hash(hash, clave);
	par_hash_t *parhash_actual = NULL;
	if (iterador) 
		parhash_actual = lista_iter_ver_actual(iterador);
	if (parhash_actual) { // Si la clave ya existia...
		lista_iter_destruir(iterador);
		if(hash->destruir_dato)
			hash->destruir_dato(parhash_actual->dato); // Borro dato ant
		parhash_actual->dato = dato;	
		return true;
		}
		
	// Si ninguno coincidio, debo insertar el par al final
	size_t posicion = funcion_hash(clave, hash->tamanio);
	par_hash_t * parhash = par_hash_crear(clave, dato); 
	if (!parhash)
		return false;
	// Si no se puede insertar_ultimo, se borra el par_hash	
	if (!lista_insertar_ultimo(hash->datos[posicion], parhash)) {	
		free(parhash->clave);
		free(parhash);
		return false;
		} 
	
	// Si salio bien, aumento el tamanio y devuelvo true
	hash->cantidad++;
	
	
	if(hay_que_redimensionar(hash)) 
		hash = redimensionar_hash(hash);
	
	
	return true;
}

// Borra el par clave-dato del hash. Devuelve el dato correspondiente
// a la clave insertada. Si la clave es invalida devuelve NULL. 
// Pre: el hash fue creado.
// Post: se elimino el par clave-dato y se devolvio el dato o NULL.
void *hash_borrar(hash_t *hash, const char *clave) {
	lista_iter_t *iterador = buscar_en_hash(hash, clave);
	if (!iterador) // Si la clave no existe, no hago nada
		return NULL; 
	// Si la clave existe, borro como esperado
	size_t posicion = funcion_hash(clave, hash->tamanio);
	par_hash_t *parhash_actual = lista_borrar(hash->datos[posicion],iterador);
	void* valor = parhash_actual->dato; 
	lista_iter_destruir(iterador);
	free(parhash_actual->clave);
	free(parhash_actual);
	hash->cantidad--;
	return valor;
	
	if(hay_que_redimensionar(hash))
		hash = redimensionar_hash(hash);
	
}

// Obtiene el valor correspondiente a la clave ingresada. Si la
// clave es invalida devuelve NULL.
// Pre: el hash fue creado.
// Post: se devuelve el dato correspondiente.
void *hash_obtener(const hash_t *hash, const char *clave) {
	lista_iter_t *iterador = buscar_en_hash(hash, clave);
	
	if(!iterador)
		return NULL;
	par_hash_t *parhash_actual = lista_iter_ver_actual(iterador);
	lista_iter_destruir(iterador);
	return parhash_actual->dato;
}

// Verifica si la clave recibida existe en el hash. Devuelve
// true si es asi o false en caso contrario.
// Pre: el hash fue creado.
// Post: se devuelve true si la clave existe en el hash o false
// en caso contrario.
bool hash_pertenece(const hash_t *hash, const char *clave) {
	return hash_obtener(hash, clave);
}

// Devuelve la cantidad de elementos almacenados en el hash.
// Pre: el hash fue creado.
size_t hash_cantidad(const hash_t *hash) {
	return hash->cantidad;
}

// Destruye el hash y todos sus elementos.
// Pre: el hash fue creado.
void hash_destruir(hash_t *hash) {
	par_hash_t *parhash_actual;
	// Primero destruyo todas las listas y sus elementos
	for (int j = 0; j < hash->tamanio; j++) {
		while(!lista_esta_vacia(hash->datos[j])) {
			parhash_actual = lista_borrar_primero(hash->datos[j]);
			if(hash->destruir_dato)
				hash->destruir_dato(parhash_actual->dato);
			free(parhash_actual->clave);
			free(parhash_actual);
			}
		lista_destruir(hash->datos[j],NULL);
	}
	// Cuando destruyo todas las listas, libero al hash
	free(hash->datos);
	free(hash);
}


/*---------------------------
    Primitivas de iterador
----------------------------*/

// Crea un iterador apuntando al hash. Devuelve NULL
// si no pudo crearse.
// Pre: el hash fue creado.
// Post: se devuelve el iterador aputando al primer
// elemento del hash o a NULL si el hash esta vacio.
hash_iter_t *hash_iter_crear(hash_t *hash) {
	// Creo un iterador y lo coloco en el primer elemento
	hash_iter_t* nuevo_iter = malloc(sizeof(hash_iter_t));
	if (!nuevo_iter) 
		return NULL;
	nuevo_iter->hash = hash;
	nuevo_iter->pos_actual = 0;
	// Itero hasta encontrar una lista no vacia
	while ((nuevo_iter->pos_actual < (hash->tamanio-1)) && lista_esta_vacia(hash->datos[nuevo_iter->pos_actual])) 
		nuevo_iter->pos_actual++;

	// Creo un iterador en la lista en la que quedé
	nuevo_iter->lista_iter = lista_iter_crear(hash->datos[nuevo_iter->pos_actual]);
		if(!nuevo_iter->lista_iter) {
			free(nuevo_iter);
			return NULL;
			}
	return nuevo_iter;
}

// Mueve el iterador a la siguiente posicion del hash.
// Pre: el iterador fue creado.
// Post: el iterador apunta al elemento siguiente de la lista.
bool hash_iter_avanzar(hash_iter_t *iter) {
	// Si esta al final, no avanzo
	if (hash_iter_al_final(iter)) 
		return false;

	// Si no estoy al final de la lista actual, trato de avanzar
	if (!lista_iter_al_final(iter->lista_iter)) {
		lista_iter_avanzar(iter->lista_iter);
		// Chequeo que no haya llegado al final con ese paso
		if (!lista_iter_al_final(iter->lista_iter))
			return true;
		}
		
	// Si estoy al final de una lista, trato de pasar a la siguiente
	// Y me encuentro en la ultima de las listas, estoy al final
	if (iter->pos_actual == (iter->hash->tamanio -1))
		return false;
	
	// Si no estoy en la ultima de las listas, itero hasta encontrar
	// alguna lista no vacia.
	while (iter->pos_actual < (iter->hash->tamanio - 1)) {
		iter->pos_actual++;
		// Encontre una lista no vacia
		if (!lista_esta_vacia(iter->hash->datos[iter->pos_actual])) {
			//Renuevo el iterador, y trato de crear uno en la nueva lista
			lista_iter_destruir(iter->lista_iter);
			iter->lista_iter = lista_iter_crear(iter->hash->datos[iter->pos_actual]);
			if (!iter->lista_iter) return false;
			return true;
		}
	}
	
	// Si llego a este punto es porque estoy en la ultima posición del arreglo
	// de listas, y esta esta vacia, o estoy al final.
	// Entonces coloco el lista_iterador en NULL.
	lista_iter_destruir(iter->lista_iter);
	iter->lista_iter = lista_iter_crear(iter->hash->datos[iter->pos_actual]);		
	return false;
}

// Obtiene la clave del elemento apuntado por el iterador.
// Pre: el iterador fue creado.
// Post: se devolvio el valor del elemento apuntado por el iterador.
const char *hash_iter_ver_actual(const hash_iter_t *iter) {
	if (hash_iter_al_final(iter)) 
		return NULL;
	return ((par_hash_t*)(lista_iter_ver_actual(iter->lista_iter)))->clave; 
}

// Devuelve verdadero o falso, segun el iterador haya 
// alcanzado o no el final del hash.
// Pre: el iterador fue creado.
bool hash_iter_al_final(const hash_iter_t *iter) {
	return (lista_iter_al_final(iter->lista_iter));
}

// Destruye el iterador. 
// Pre: el iterador fue creado.
// Post: se destruyo el iterador.
void hash_iter_destruir(hash_iter_t* iter) {
	lista_iter_destruir(iter->lista_iter);
	free(iter);
}
