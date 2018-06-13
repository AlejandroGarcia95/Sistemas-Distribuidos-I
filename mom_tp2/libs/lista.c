#include "lista.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* ******************************************************************
 *                DEFINICION DE LOS TIPOS DE DATOS
 * *****************************************************************/

/* La lista está planteada como una lista de punteros genéricos. */

struct _nodo{
	void *dato;
	struct _nodo *puntero;
};

typedef struct _nodo nodo_t;

struct lista {
	nodo_t *primero, *ultimo;
	size_t tamanio;
};

struct lista_iterador {
	nodo_t *actual, *anterior;
};

// Declaro funciones auxiliares a usar (detalladas abajo)

nodo_t *nodo_crear();

void lista_insertar_lista_vacia(lista_t* lista, nodo_t* nodo);

bool iterador_apunta_primero(lista_t* lista, lista_iter_t *iter);


/* ******************************************************************
 *                    PRIMITIVAS DE LA LISTA
 * *****************************************************************/

// Crea una lista.
// Post: devuelve una nueva lista vacía.
lista_t* lista_crear() {
	lista_t *p_lista=malloc(sizeof(lista_t));
	if(p_lista==NULL) {
		fputs("Error, no hay memoria", stdout);
		return NULL;
		}
	p_lista->tamanio=0;	
	p_lista->primero=NULL;
	return p_lista;
	
}

// Destruye la lista. Si se recibe la función destruir_dato por parámetro,
// para cada uno de los elementos de la lista llama a destruir_dato.
// Pre: la lista fue creada. destruir_dato es una función capaz de destruir
// los datos de la lista, o NULL en caso de que no se la utilice.
// Post: se eliminaron todos los elementos de la lista.
void lista_destruir(lista_t *lista, void destruir_dato(void*)) { //TE TENGO MIEDO
	while(!lista_esta_vacia(lista)) {
		if(destruir_dato) 
			destruir_dato(lista_ver_primero(lista));
		lista_borrar_primero(lista);
	}
	free(lista);	
}


// Devuelve verdadero o falso, según si la lista tiene o no elementos.
// Pre: la lista fue creada.
bool lista_esta_vacia(const lista_t *lista) {
	return lista->tamanio==0;
}

// Agrega un nuevo elemento a la lista en la primera posicion. Devuelve falso en caso de error.
// Pre: la lista fue creada.
// Post: se agregó un nuevo elemento a la lista, dato se encuentra al principio
// de la lista.
bool lista_insertar_primero(lista_t *lista, void* dato) {
	nodo_t *minodo=nodo_crear(); 
	if(minodo==NULL) 
		return false;
	
	minodo->dato=dato;
	if(lista_esta_vacia(lista))
		lista_insertar_lista_vacia(lista, minodo);
	else {
		minodo->puntero=lista->primero;
		lista->primero=minodo;
		}
	
	lista->tamanio+=1;
	return true;
}

// Agrega un nuevo elemento a la lista en la ultima posicion. Devuelve falso en caso de error.
// Pre: la lista fue creada.
// Post: se agregó un nuevo elemento a la lista, dato se encuentra al final
// de la lista.
bool lista_insertar_ultimo(lista_t *lista, void* dato) {
	nodo_t *minodo=nodo_crear();
	if(minodo==NULL) 
		return false;
	
	minodo->dato=dato;
	if(lista_esta_vacia(lista))
		lista_insertar_lista_vacia(lista, minodo);
	else {
		lista->ultimo->puntero=minodo;
		lista->ultimo=minodo;
		minodo->puntero=NULL;
	}
	
	lista->tamanio+=1;
	return true;
}

// Obtiene el valor del primer elemento de la lista. Si la lista tiene
// elementos, se devuelve el valor del primero, si está vacía devuelve NULL.
// Pre: la lista fue creada.
// Post: se devolvió el primer elemento de la lista, cuando no está vacía.
void* lista_ver_primero(const lista_t *lista) {
	return lista_esta_vacia(lista) ? NULL : lista->primero->dato;
	/*if(lista_esta_vacia(lista))
		return NULL;
	return lista->primero->dato;*/
}

// Saca el primer elemento de la lista. Si la lista tiene elementos, se quita el
// primero de la lista, y se devuelve su valor, si está vacía, devuelve NULL.
// Pre: la lista fue creada.
// Post: se devolvió el valor del primer elemento anterior, la lista
// contiene un elemento menos, si la lista no estaba vacía.
void *lista_borrar_primero(lista_t *lista) {
	void *valor;
	if(lista_esta_vacia(lista))
		return NULL;
	valor=lista_ver_primero(lista);
	nodo_t *punt_aux=lista->primero;
	if(lista_largo(lista)>1)  
		lista->primero=lista->primero->puntero;
	else   //Si la lista tiene un unico elemento simplemente tengo que
		lista->primero=NULL;  //eliminarlo y arreglar lista->primero
		
	free(punt_aux);		
	lista->tamanio-=1;
	return valor;
}

//Devuelve el tamaño de la lista (cantidad de elementos).
//Pre: la lista fue creada.
//Post: se devolvio la cantidad de elementos de la lista en O(1).
size_t lista_largo(const lista_t *lista) {
	return lista->tamanio;
}

//--------------------------------
// PRIMITIVAS CON ITERADORES
//--------------------------------

// Crea un iterador externo apuntando al primer elemento de la lista. Si la lista esta
// vacia, el iterador actual apunta a NULL.
// Pre: la lista fue creada.
// Post: devuelve un nuevo iterador apuntando al primer elemento de la lista o a NULL si
// estaba vacia.
lista_iter_t *lista_iter_crear(const lista_t *lista) {
	lista_iter_t* iterador= malloc(sizeof(lista_iter_t));
	if(iterador==NULL) {
		fputs("Error, no hay memoria", stdout);
		return NULL;
	}
	
	if(lista_esta_vacia(lista))
		iterador->actual=NULL;
	else
		iterador->actual=lista->primero;
	iterador->anterior=NULL;	
	return iterador;
}

// Mueve el iterador a la siguiente posicion de la lista. Si el iterador ya esta en
// el final de la lista o si esta estaba vacia, devuelve false; si pudo 
// apuntar al siguiente elemento, devuelve true.
// Pre: el iterador fue creado.
// Post: el iterador apunta al elemento siguiente de la lista y devuelve true, o 
// devuelve false si el iterador ya estaba en el final de la lista o la lista
// es una lista vacia.
bool lista_iter_avanzar(lista_iter_t *iter) {
	if(lista_iter_al_final(iter))
		return false;
	iter->anterior=iter->actual;
	iter->actual=iter->actual->puntero;
	return true;
}

// Obtiene el valor del elemento apuntado por el iterador. Si la lista esta
// vacia o el iterador ya esta en el final de la lista, devuelve NULL.
// Pre: el iterador fue creado.
// Post: se devolvio el valor del elemento apuntado por el iterador, o NULL
// si la lista estaba vacia o el iterador ya la recorrio por completo.
void *lista_iter_ver_actual(const lista_iter_t *iter) {
		return lista_iter_al_final(iter) ? NULL : iter->actual->dato;
		/*if(lista_iter_al_final(iter)) 
			return NULL;
		return iter->actual->dato;*/
}

// Devuelve verdadero o falso, segun el iterador haya alcanzado o no el final de la lista.
// Si la lista esta vacia devuelve tambien false.
// Pre: el iterador fue creado
bool lista_iter_al_final(const lista_iter_t *iter) {
	return iter->actual==NULL;  //Pasa si la lista esta vacia o ya apunto al NULL del ultimo
}

// Destruye el iterador.
// Pre: el iterador fue creado.
void lista_iter_destruir(lista_iter_t *iter) {
	free(iter);
}

// Inserta en la lista un elemento en la posicion actual a la que apunta el iterador. Si la lista
// esta vacia, agrega el elemento como el primero de la lista. Si el iterador ya recorrio la lista
// por completo, inserta el elemento al final de esta.
// Pre: el iterador fue creado.
// Post: el elemento fue insertado en la lista, en la posicion en la que apunta el iterador,
// y el elemento que antes ocupaba su lugar se encuentra en la posicion siguiente a esa.
bool lista_insertar(lista_t *lista, lista_iter_t *iter, void *dato) {
	nodo_t *minodo=nodo_crear();
	if(minodo==NULL) 
		return false;
		
	minodo->dato=dato;
	if(lista_esta_vacia(lista))
		lista_insertar_lista_vacia(lista, minodo);
	else {
		minodo->puntero=iter->actual; //Apunta al nodo que antes estaba en esa pos.
		if(iterador_apunta_primero(lista, iter)) //Esto pasa si la lista tiene un unico elemento o el
			lista->primero=minodo; //iterador apuntaba al primer elemento.
		else {
			iter->anterior->puntero=minodo;
			if(lista_iter_al_final(iter)) 
				lista->ultimo=minodo; //Solo pasa si inserte un elemento al final.
			}
		}

	iter->actual=minodo;
	lista->tamanio+=1;
	return true;		
}

// Borra de la lista el elemento al cual apuntaba iter. Si la lista ya esta vacia o si el
// iterador la recorrio por completo, devuelve NULL.
// Pre: el iterador fue creado.
// Post: el elemento fue borrado de la lista, e iter->actual apunta al que antes era el 
// elemento siguiente a ese. Si no habia elemento que borrar, se devolvio NULL.
void *lista_borrar(lista_t *lista, lista_iter_t *iter) {
		if(lista_iter_al_final(iter)) // Si la lista esta vacia o iter esta en el 
			return NULL; // final, no borro nada
		
		void *valor=lista_iter_ver_actual(iter);
		//Si lista tiene largo=1, o iter->actual apunta al primero, iter->anterior estara aputando a NULL
		if(iterador_apunta_primero(lista, iter)) {
				iter->actual=iter->actual->puntero;
				free(lista->primero);//Hay que arreglar tambien el puntero primero de la lista
				lista->primero=iter->actual;
				}
		else { //Es lo mismo tanto si apunta al "medio" como al ultimo elemento de la lista
				iter->anterior->puntero=iter->actual->puntero;
				free(iter->actual);
				iter->actual=iter->anterior->puntero;
				}
				
		lista->tamanio -= 1;
		if(lista_iter_al_final(iter)) //Solo si recien borre el ultimo elemento
			lista->ultimo=iter->anterior;
		if(lista_esta_vacia(lista))  //Solo si recien vacie la lista 
			lista->ultimo=NULL; 
		
		return valor;
}

// Itera internamente para aplicar la funcion visitar a los elementos de la lista, segun
// este definida. Puede recibir ademas un extra por si necesito algun dato adicional. Si
// la lista esta vacia, o si ya la recorri por completo, debe terminar la iteracion.
// Pre: la lista esta creada, visitar es una funcion con esa firma y que devuelve true si
// desea seguir iterando y false en caso contrario.
void lista_iterar(lista_t *lista, bool (*visitar)(void *dato, void *extra), void *extra) {
		if(lista_esta_vacia(lista))
			return;
		nodo_t *punt_aux = lista->primero;

	/*La iteracion la debo hacer mientra visitar me lo siga indicando y ademas mientras no haya
	leido todos los elementos de la lista. Si punt_aux es NULL entonces llegue al final.*/
		while((punt_aux!=NULL)&&(visitar(punt_aux->dato, extra))) { 
			punt_aux = punt_aux->puntero; 
		}

}


/*------------------------------
------FUNCIONES AUXILIARES------
------------------------------*/

// Crea un nodo y ademas manda un mensaje de error si no hay memoria
nodo_t *nodo_crear() {
	nodo_t *minodo=malloc(sizeof(nodo_t));
	if(minodo==NULL) 
		fputs("Error, no hay memoria", stdout);
	return minodo;
}

// Recibe una lista vacia y un nodo- Hace que lista->primero y
// lista->ultimo apunten ambos a dicho nodo.
void lista_insertar_lista_vacia(lista_t* lista, nodo_t* minodo) {
	lista->primero=minodo;
	lista->ultimo=minodo;
	minodo->puntero=NULL;
}

// Checkea si el iterador apunta al mismo nodo que lista->primero
bool iterador_apunta_primero(lista_t* lista, lista_iter_t *iter) {
	return lista->primero==iter->actual;
}
