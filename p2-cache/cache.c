/*
 * cache.c
 */


// Las divisiones se ven mejor en WIDESCREEEN haha, para que no te hagas bolas con los mments y asi hay que usarlo así.
// El inicio y fin de cada macrochunk o funcion tiene tres lineas, para que sea mas fácil moverse en el codigo
// El archivo .h o header te da una idea abstracta del las estructuras y funciones, para no perdernos entre tanto pinche comentario hahaha
// Uso indistintamente linea de bloque, lo siento pero no hay tiempo de cambiarlo ¡¡¡¡¡¡¡¡¡¡¡¡ LINEA = BLOQUE !!!!!!!!!!!!!!!!!!!!!!!!!
// Para mapeo directo associative es 1, pues equivale a que cada linea sea un set, solo que se identifica con index, no con set de index; es solo notacion
// Los tamaños estan definidos en BYTES!!!!!!!!!!!!! revisar el header
// ADDRESS de la memoria principal : -xffffffff , que equivale a addres de 32 bits


/* En el header estan los tipos cache, cache_stat & cache_line con sus respectivos pointers agregando una P previa, ademas del amcro de log_2. No es ln :( */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "cache.h"
#include "main.h"

/********************************  cache configuration parameters *****************************************************************/
/********************************  cache configuration parameters *****************************************************************/
/********************************  cache configuration parameters *****************************************************************/

    // Responden a los parametros del header para la primera prueba


static int cache_split = 0;   // --------------------- 0 para cache unificado, o.w no unificado

    // Tamaños de cache. Probablemente en caso de ser unificado, el tamaño de los dos caches sea el mismo
    // u=i=d. Estar atento a politicas de write, pues no aplican para el de instrucciones!!!!
  // Son default pq el cache esta unificado para la primera practica

static int cache_usize = DEFAULT_CACHE_SIZE;  //------ tamaño de cache de unificado
static int cache_isize = DEFAULT_CACHE_SIZE;  //------ tamaño de cache de instrucciones
static int cache_dsize = DEFAULT_CACHE_SIZE;  //------ tamaño de cache de datos



    // Atributos de bloque
static int cache_block_size = DEFAULT_CACHE_BLOCK_SIZE;
static int words_per_block = DEFAULT_CACHE_BLOCK_SIZE / WORD_SIZE;


  // Tipo de cache k-way
static int cache_assoc = DEFAULT_CACHE_ASSOC; // mapeo directo es 1-way



    // POliticas de write
static int cache_writeback = DEFAULT_CACHE_WRITEBACK;
static int cache_writealloc = DEFAULT_CACHE_WRITEALLOC;


  /* cache model data structures */
    // --------------------------------- icache=dcache=ucache --------------------------------
    // Super importante hacer las funciones tomando esto en cuenta, no se nos olvide las politicas de write
    // Notar que para llamar a cada tipo de cache usaremos pointers
    // agregué un Pcache para ucache para el caso unificado, u=d=i

static Pcache icache; 
static Pcache dcache;
static Pcache ucache;


    // Cache NO-split-> tratar a c2 como null;
    // Cache split -> c1= instr, c2 = datos
static cache c1;  // cache unificado, y cache de instrucciones
static cache c2;  // cache de datos, o NULL en caso de ser unificado


    // Variables de estadisticas
    // agregué un cache_stat para ucache para el caso unificado, u=d=i
static cache_stat cache_stat_inst; // stats para cache de instrucciones o cache unificado
static cache_stat cache_stat_data; // stats para cache de datos (null en caso de unificado)
static cache_stat cache_stat_u;    // stats para el cache unificado (igual a las dos de abajo, todo es c1 en este caso)
 


/******************************************* END cache configuration parameters **************************************************************/
/******************************************* END cache configuration parameters **************************************************************/
/******************************************* END cache configuration parameters **************************************************************/






/*-------------------------------------------- Función set_cache_param ------------------------------------------------------------------- */
/*-------------------------------------------- Función set_cache_param ------------------------------------------------------------------- */
/*-------------------------------------------- Función set_cache_param ------------------------------------------------------------------- */



/* 

Función para definir la estructura de la memoria cache que se simulará. Recibe (atributo y valor). De tal manera que los parametros con los que se 
construrá la estructura cache correspondan a lo deseado. Para ver una descripción especifica ver cada caso. 
1 : La función busca el atributo de input, y coloca los valores pertinentes a la estructura.

W: No es necesario modificarla, es apra pasar argumentos, revisar main

*/

void set_cache_param(param, value)
  int param;
  int value;
{

  switch (param) {

    // (Para definir el tamaño del bloque & las palabras por bloque) Notar que utiliza la varaible globlal WORD_SIZE  
  case CACHE_PARAM_BLOCK_SIZE:
    cache_block_size = value;             // tamaño del bloque en bytes
    words_per_block = value / WORD_SIZE; // #de palabras = tamaño del bloque / tamaño de las palabras
    break;
  

    // Para cache unificado
  case CACHE_PARAM_USIZE:
    cache_split = FALSE;
    cache_usize = value;
    break;

  
    // Tamaño del cache especializado en INSTRUCCIONES y si existe
  case CACHE_PARAM_ISIZE:
    cache_split = TRUE;   //existe cache para instrucciones
    cache_isize = value;  // tamaño del cache de datos
    break;

  
     // Tamaño del cache especializado en DATOS y si existe
  case CACHE_PARAM_DSIZE:
    cache_split = TRUE;   // existe cache especializado para datos
    cache_dsize = value;  // tamaño del cache para datos
    break;

  
    // Por ahora 1, solo MAPEO DIRECTO
  case CACHE_PARAM_ASSOC:
    cache_assoc = value;
    break;
  

  /* ----------------Politicas para escribir a memoria dependen del hit o miss ------------------------- */  
   
      // Politica WRITEBACK activa----> Comparte valor con wel parametro Writethrough
      // Verdadero si es politca writeback
  case CACHE_PARAM_WRITEBACK:
    cache_writeback = TRUE;
    break;
  
      // Politica WRITETHROUGH activa----> Comparte valor con wel parametro WriteBACK
      // FALSE si es politca writethrough
  case CACHE_PARAM_WRITETHROUGH:
    cache_writeback = FALSE;
    break;
  
      // Politica WRITEALLOC activa----> Comparte valor con wel parametro Nowritealloc
      // TRUE si es politca writealloc
  case CACHE_PARAM_WRITEALLOC:
    cache_writealloc = TRUE;
    break;
  
      // Politica NOWRITEALLOC activa----> Comparte valor con wel parametro writealloc
      // FALSE si es politca NOwritealloc
  case CACHE_PARAM_NOWRITEALLOC:
    cache_writealloc = FALSE;
    break;
  
      // Importante para debuggear
  default:
    printf("error set_cache_param: bad parameter value\n");
    exit(-1);
  }

}
/*-------------------------------------------- End Función set_cache_param ------------------------------------------------------------------- */
/*-------------------------------------------- End Función set_cache_param ------------------------------------------------------------------- */
/*-------------------------------------------- End Función set_cache_param ------------------------------------------------------------------- */








/*-------------------------------------------------init_cache : = Inicializar cache(s)*---------------------------------------------------------*/
/*-------------------------------------------------init_cache : = Inicializar cache(s)*---------------------------------------------------------*/
/*-------------------------------------------------init_cache : = Inicializar cache(s)*---------------------------------------------------------*/


/* initialize the cache, and cache statistics data structures */
/* Por DEFAULT c1 es cache unificado, cuando exista split se agregara c2 para cahce de datos.  */
/* Hay que setear las variables de estadistica a 0 para evitar errores  */


/*  Observaciones generales para el metodo init_cache*/

    // Primero cache unificado, falta revisar si agregamos split (enviar mail)
    // Despues agregaremos split, que consitiría en crear dos caches, por lo que hay que modificar su lectura

    // Por DEFAULT c1 es cache unificado, cuando exista split se agregara c2 para cache de datos. 

    // Tenemos que hacer para cache unificado:  datos = cache instrucciones = cache unificado y que las funciones repliquen lo mismo
    // hay que ver nuestro diseñño de write back y allocate. para que no choquen los caches de instrucciones y datos si hacemos
    // lo anterior. Podemos agregar una bandera para  que no ocurra con la parte de instrucciones. Vamos viendo como se desarrollan
    // las funciones.

void init_cache()
{
 
  /* Construir (initialize) los caches */

    // En este caso solo será para cache unificadp

      // Bits para @ de cache unificadao 1-way, CAMBIARAAAAN!!!!!!!!!!
      // unsigned s_offset = LOG2(words_per_block) // tamaño del offset (log(words per block))
      // unsigned s_addres = LOG2(cache_usize) // tamaño del address (log(numero de bloques))


      
    /* --------------------------------------------------------------------CACHE -----------------------------------------------------------*/
      //  Se define cache unificado c1; los pointers de cada tipo de cache apuntan a la estructura de c1
      ucache = icache = dcache = &c1; 

      /*  Necesitamos: size, associativity, n_sets, mask_tag, mask_index, mask_offset, (*) LRU_head, (*) LRU_tail, (*) set_contents, contents */
      //Dado que aqui son iguales podriamos usar al notacion c1.size = .... c1.atributo

    /* --------------------------------------------------------------------size -----------------------------------------------------------*/


      // tamaño del cache es int, representado por size
      ucache -> size = cache_usize;    
       printf(" Size : %d ",c1.size);

    /* --------------------------------------------------------------------Assoosiativity -----------------------------------------------------------*/

      // k-way es int (1:= mapeo directo)      
      ucache -> associativity = cache_assoc; 
      
    /* --------------------------------------------------------------------n_sets -----------------------------------------------------------*/

      //numero de sets es entero n_sets
      // numero de sets = (tamaño del cache)/(tamaño del set) & tamaño del set = (numero de bloques por set)*(tamaño del bloque)
      // notar que mapeo directo equivale a que cada linea sea un set, y el index el index del set
      ucache -> n_sets = c1.size/(cache_assoc*cache_block_size); 
      printf(" Sets : %d ",c1.n_sets);
    
    /* --------------------------------------------------------------------masks & sizes -----------------------------------------------------------*/
      // esta en el example de masks el proceso para corroborar que es correcto
      // index_mask (mascara para encontrar el indice)
      // es unsigned
      // para 1-way es igual al log(number of sets)
      // ver ejemplo de mask


int bits_o[32];
      int bits_i[32];
       int bits_t[32];




      
		ucache -> offset_s = LOG2(words_per_block*4 ); // bits para el offset
		ucache -> index_s = LOG2((*ucache).n_sets); // bits para el index
		ucache -> tag_s =  32 - (*ucache).offset_s - (*ucache).index_s; // bits para el tag

       ucache -> offset_mask = 0xffffffff >> (32- ((*ucache).offset_s) ); // calcula la mascara del offset
       ucache -> index_mask = 0xffffffff >> (32 - ((*ucache).index_s)); 	// calcula la mascara del index, pero sin tomar encuenta la del offset
       ucache -> index_mask = ucache -> index_mask << ((*ucache).offset_s ); // toma encuenta la amscara del offset
       ucache -> tag_mask  = 0xffffffff << ((*ucache).offset_s  + (*ucache).index_s ) ;  // calcula la mascara del tag
                   

    for (int i = 0; i < 32; i = i+1)
    {
        bits_o[i] = (c1.offset_mask >> i) & 1;
        bits_i[i] = (c1.index_mask >> i) & 1;
        bits_t[i] = (c1.tag_mask >> i) & 1;
        
        
    }
	  printf("\n");

/*
  printf("off:");
	    for (int i = 31; i >-1; i = i-1) {
        printf("%d",bits_o[i] );
    }
	printf("\n");


	
    printf("ind:");
    	    for (int i = 31; i >-1; i = i-1) {
        printf("%d",bits_i[i] );
    }
	printf("\n");


    
    printf("tag:");
    	    for (int i = 31; i >-1; i = i-1) {
        printf("%d",bits_t[i] );
    }
	printf("\n");
    
*/
    /* --------------------------------------------------------------------LRU_head ------------------------------------------------------------------*/


      // LRU_head vector con los apuntadores a la direccion del primer cache_line de cada set ----> vector de apuntadores a @(head de cada set)
      
      /*  Ver el ejemplo que hice de exp.c
          1) En .h, despues de definir la estructura de cache_line_, viene *Pcache_line, esto permite crear apuntadores para las estructuras de tipo
            cache_line --->   cache_line cl[]; // crea un vector de cache_line 
                              Pcache_line *p_cl; //donde p_cl es un apuntador que permite invocar objetos de tipo cache_line
                              p_cl = & cl; // p_cl apunta al primer elemento del vector cl
                              *p_cl; // regresa el primer objeto del vector cl
                              *(1+p_cl); // regresa el segundo objeto del vector ....

          A) LRU_head es un apuntador para lineas de cache, especificamente heads de set, 
          por lo que *(LRU_head) me regresa el cahe_line que es head del primer set o set0
          *(LRU_head + 1) me regresa el cache_line que es head del segundo set o set1 ...

          malloc me reserva el espacio de memoria que requiero, en este caso un vector con apuntadores de headers que son de tipo cache_line, y me regresa la dirreción del primer
          elemento de dicho vector (donde comienza). Como el return  de malloc es una direccion es necesario agregar el casting (Pcache_line *) de tal forma que esa direccion se
          convierta en un objeto del tipo (Pcache_line *) y pueda ser asignada a LRU_head

      */

      // LRU_head termina en convertirse en un vector de apuntadores hacia los headers de la cache. Para direct solo hay una cabeza
      // Se pudo utilizar (ucache -> n_sets) para definir el vector, pero esta forma general la sugiere el pdf de maryland
      ucache->LRU_head = (Pcache_line *)malloc(sizeof(Pcache_line)*ucache->n_sets);

      // For para inicializar los headers con NULL
      // en mapeo directo tail=head pues cada set es una linea
      

      for( int a ; a <ucache -> n_sets; a++)
        {
          ucache->LRU_head[a] = NULL ; 
        }

       
    /* --------------------------------------------------------------------LRU_tail ----------------------------------------------------------------------*/


      // LRU_tail vector con los apuntadores a ultimo cache_line de cada set ----> vector de apuntadores a tail de cada set (no existen en one way)
      ucache->LRU_tail = (Pcache_line *)malloc(sizeof(Pcache_line)*ucache->n_sets);

       
      // For para inicializar las tails con NULL
      // en mapeo directo tail=head pues cada set es una linea
      for(int a ; a<ucache -> n_sets; a++)
        {
         ucache->LRU_tail[a] = NULL ; 
        }

    /* --------------------------------------------------------------------set_contents -----------------------------------------------------------*/
      
      // set_contents: number of valid entries in set. 
      // Es un apuntador para enteros usar prefijo (int *)
     
      ucache -> set_contents = (int *)malloc(sizeof(int)*ucache->n_sets);
    
    /* --------------------------------------------------------------------contents-----------------------------------------------------------------*/

      // contents : number of valid entries in the cache
     ucache -> contents = 0;






  /*  END Construir (initialize) los caches */



  /* Definicion de las estadisticas */

    /*  Heder indica 5 atributos: accesses, misses, replacements, demand_fetches, copies_back.
        Hay que crear una structura cache_stat para cada memoria {U,I,D}, pues las stats no estan la estructura del cache son independientes
        porque en la tabla hay que reportar cada uno por separado, y para facilitar cuando agreguemos split.
        Para el caso unificado los valores de todos tiene que ser el mismo, ojo que el de instrucciones no tiene politicas de write iguales,
        pero al hacerlos iguales lo tendrán. EN caso de hacer split seran diferentes
    */

      cache_stat_inst.accesses        = 0 ;
      cache_stat_inst.misses          = 0 ;
      cache_stat_inst.replacements    = 0 ;
      cache_stat_inst.demand_fetches  = 0 ;
      cache_stat_inst.copies_back     = 0 ;


      cache_stat_data.accesses        = 0 ;
      cache_stat_data.misses          = 0 ;
      cache_stat_data.replacements    = 0 ;
      cache_stat_data.demand_fetches  = 0 ;
      cache_stat_data.copies_back     = 0 ;

      cache_stat_u.accesses        = 0 ;
      cache_stat_u.misses          = 0 ;
      cache_stat_u.replacements    = 0 ;
      cache_stat_u.demand_fetches  = 0 ;
      cache_stat_u.copies_back     = 0 ;

      printf("\n  Create cache \n");

  /* End dDefinicion de estadisticas*/



}


/*------------------------------------------------- End init_cache : = Inicializar cache(s)*---------------------------------------------------------*/
/*------------------------------------------------- End init_cache : = Inicializar cache(s)*---------------------------------------------------------*/
/*------------------------------------------------- End init_cache : = Inicializar cache(s)*---------------------------------------------------------*/










/************************************************************/
  /* handle an access to the cache */

// ESta funcion lee las trazas y recibe direccón y numero como string
// posteriormente las convierte a entero y la direccion a un enetero sin signo para que las direcciones quepan en 32 bits


void perform_access(addr, access_type)
  unsigned addr, access_type;
{	
	switch(access_type){

		case 0:

		perform_access_d_load(addr);

		break;

		case 1:

		perform_access_d_write(addr);

		break;

		case 2:

		perform_access_i(addr);

		break;

	}
	
}



/************************************************************/



void perform_access_i(unsigned addr)
{		
	//printf("Instruction:");
	// Crea los saltos = el valor del index
	// Crea el tag

	 unsigned jump = (addr & (c1.index_mask) ) >> ( c1.offset_s  ); 
	 unsigned t_tag = (addr & (c1.tag_mask) ) >> (c1.index_s + c1.offset_s  ); 
	 //printf(" I : j-%u , t-%u\n", jump, t_tag);
	 cache_stat_inst.accesses ++; // incrementa el contador de accesos a cache

 	if( c1.LRU_head[jump] == NULL) // Linea de cache correspondiente al index esta vacia
 	{	//printf("NULL \n");
 		c1.LRU_head[jump] = malloc(sizeof(cache_line)); //crea la nueva linea de cache y la guarda en LRU correspondiente
 		c1.LRU_head[jump]->tag = t_tag; // le de el tag correspondiente a la linea
 		c1.LRU_head[jump]->dirty = 0; // le de el tag correspondiente a la linea

 		cache_stat_inst.misses ++; // incrementa el contador miss
 		cache_stat_inst.demand_fetches += words_per_block; // incrementa el contador miss ++; // incrementa el contador de fetches


 	} else if( c1.LRU_head[jump]->tag == t_tag) // Linea de cache correspondiente al index esta cargada, match de tags
 			{
 				//printf("HIT \n");


 			} else // linea de cache correspondiente al index esta ocupada, pero el tag no corresponde
 				{	//printf("TAG \n");
 					// para ver el dirty bit
 					if(c1.LRU_head[jump]->dirty == 1) // revisa el dirty bit en caso de que tenga que escribir el dato en memoria
 					{
 						cache_stat_data.copies_back +=  words_per_block;

 					}

 				c1.LRU_head[jump]->tag = t_tag; // le de el tag correspondiente a la linea
 				c1.LRU_head[jump]->dirty = 0; // le de el tag correspondiente a la linea

 				
 				cache_stat_inst.misses ++; // incrementa el contador miss
 				cache_stat_inst.demand_fetches += words_per_block; // incrementa el contador miss ++; // incrementa el contador de fetches
 				cache_stat_inst.replacements++;
	


 				}



}


/************************************************************/
/************************************************************/
/************************************************************/






void perform_access_d_load(unsigned addr)  // equivalente a read data
{		//printf(" Load \n");
	// Crea los saltos = el valor del index
	// Crea el tag
	 unsigned jump = (addr & c1.index_mask ) >> ( c1.offset_s  ); 
	 unsigned t_tag = (addr & c1.tag_mask ) >> (c1.index_s + c1.offset_s  ); 
	 //printf(" L : j-%u , t-%u\n", jump, t_tag);
	cache_stat_data.accesses ++; // incrementa el contador de accesos a cache


 	if( c1.LRU_head[jump] == NULL) // Linea de cache correspondiente al index esta vacia
 	{
 		c1.LRU_head[jump] = malloc(sizeof(cache_line)); //crea la nueva linea de cache y la guarda en LRU correspondiente
 		c1.LRU_head[jump]->tag = t_tag; // le de el tag correspondiente a la linea
 		c1.LRU_head[jump]->dirty = 0; // le de el tag correspondiente a la linea

 		cache_stat_data.misses ++; // incrementa el contador miss
 		cache_stat_data.demand_fetches += words_per_block; // incrementa el contador miss ++; // incrementa el contador de fetches

 	} else if(c1.LRU_head[jump]->tag == t_tag) // Linea de cache correspondiente al index esta cargada, match de tags
 			{



 			} else // linea de cache correspondiente al index esta ocupada, pero el tag no corresponde
 				{
 					// para ver el dirty bit
 					if(c1.LRU_head[jump]->dirty == 1) // revisa el dirty bit en caso de que tenga que escribir el dato en memoria
 					{
 						cache_stat_data.copies_back +=   words_per_block;

 					}

 				c1.LRU_head[jump]->tag = t_tag; // le de el tag correspondiente a la linea
 				c1.LRU_head[jump]->dirty = 0; // le de el tag correspondiente a la linea
 				cache_stat_data.misses ++; // incrementa el contador miss
				cache_stat_data.demand_fetches += words_per_block; // incrementa el contador miss ++; // incrementa el contador de fetches
				cache_stat_data.replacements++;


 				}



}


/************************************************************/


void perform_access_d_write(unsigned addr) //
{		//printf("\n  Access Write \n");
	// Crea los saltos = el valor del index
	// Crea el tag
	 unsigned jump = (addr & c1.index_mask ) >> ( c1.offset_s  ); 
	 unsigned t_tag = (addr & c1.tag_mask ) >> (c1.index_s + c1.offset_s  ); 
	 cache_stat_data.accesses ++; // incrementa el contador de accesos a cache
	  //printf(" W : j-%u , t-%u\n", jump, t_tag);

 	if( c1.LRU_head[jump] == NULL) // Linea de cache correspondiente al index esta vacia
 	{
 		c1.LRU_head[jump] = malloc(sizeof(cache_line)); //crea la nueva linea de cache y la guarda en LRU correspondiente
 		c1.LRU_head[jump]->tag = t_tag; // le de el tag correspondiente a la linea
 		c1.LRU_head[jump]->dirty = 1; // le de el tag correspondiente a la linea

 		cache_stat_data.misses ++; // incrementa el contador miss
 		cache_stat_data.demand_fetches += words_per_block; // incrementa el contador miss ++; // incrementa el contador de fetches


 	} else if(c1.LRU_head[jump]->tag == t_tag) // Linea de cache correspondiente al index esta cargada, match de tags
 			{
 				c1.LRU_head[jump]->dirty = 1; // pone 1 en dirty bit para avisar que lo modifico


 			} else // linea de cache correspondiente al index esta ocupada, pero el tag no corresponde
 				{
 					// para ver el dirty bit
 					if(c1.LRU_head[jump]->dirty == 1) // revisa el dirty bit de la linea que ocupa el index en caso de que tenga que escribir el dato en memoria
 					{
 						cache_stat_data.copies_back +=  words_per_block; // modifica la estadistica

 					}

 					//sustituye la linea a cache
 				c1.LRU_head[jump]->tag = t_tag; // le de el tag correspondiente a la linea
 				c1.LRU_head[jump]->dirty = 1; // le de el tag correspondiente a la linea
	
 				cache_stat_data.misses ++; // incrementa el contador miss
 				cache_stat_data.demand_fetches += words_per_block; // incrementa el contador miss ++; // incrementa el contador de fetches
 				cache_stat_data.replacements++;

 				}



}



/************************************************************/

/************************************************************/
// Cuando no existan mas instrucciones en RAM, flush vacia la memoria cache de tal manera que escribe en memoria todo lo que tenga dirty bit =1

void flush()
{ 

  Pcache_line current_line ;

  for(int i = 0; i < (c1.n_sets) ; i++) // pasa por todo el cache
  {

  	

  	if(c1.LRU_head[i] != NULL)
  	{
  	current_line = c1.LRU_head[i];  // guarda la linea actual
  	cache_stat_data.copies_back += (current_line-> dirty) * words_per_block; //modifica la estadistica
    }

  }

}
/************************************************************/






/************************************************************/
/* 
	delete : 
	removes an item from a linked list.assume a doubly linked list (which our data structures provide) and take three parameters:
	a head pointer (passed by reference), a tail pointer (passed by reference), and a pointer to the cache
	line to be inserted or deleted (passed by value).
*/

void delete(head, tail, item)
  Pcache_line *head, *tail;
  Pcache_line item;
{
  if (item->LRU_prev) {
    item->LRU_prev->LRU_next = item->LRU_next;
  } else {
    /* item at head */
    *head = item->LRU_next;
  }

  if (item->LRU_next) {
    item->LRU_next->LRU_prev = item->LRU_prev;
  } else {
    /* item at tail */
    *tail = item->LRU_prev;
  }
}
/************************************************************/





/************************************************************/
/* inserts at the head of the list */

/* 
	insert : 
	inserts an item at the head of a linked list.assume a doubly linked list (which our data structures provide) and take three parameters:
	a head pointer (passed by reference), a tail pointer (passed by reference), and a pointer to the cache
	line to be inserted or deleted (passed by value).
*/
void insert(head, tail, item)
  Pcache_line *head, *tail;
  Pcache_line item;
{
  item->LRU_next = *head;
  item->LRU_prev = (Pcache_line)NULL;

  if (item->LRU_next)
    item->LRU_next->LRU_prev = item;
  else
    *tail = item;

  *head = item;
}
/************************************************************/







/************************************************************/
void dump_settings()
{
  printf("*** CACHE SETTINGS ***\n");
  if (cache_split) {
    printf("  Split I- D-cache\n");
    printf("  I-cache size: \t%d\n", cache_isize);
    printf("  D-cache size: \t%d\n", cache_dsize);
  } else {
    printf("  Unified I- D-cache\n");
    printf("  Size: \t%d\n", cache_usize);
  }
  printf("  Associativity: \t%d\n", cache_assoc);
  printf("  Block size: \t%d\n", cache_block_size);
  printf("  Write policy: \t%s\n", 
	 cache_writeback ? "WRITE BACK" : "WRITE THROUGH");
  printf("  Allocation policy: \t%s\n",
	 cache_writealloc ? "WRITE ALLOCATE" : "WRITE NO ALLOCATE");
}
/************************************************************/





/************************************************************/
void print_stats()
{
  printf("\n*** CACHE STATISTICS ***\n");

  printf(" INSTRUCTIONS\n");
  printf("  accesses:  %d\n", cache_stat_inst.accesses);
  printf("  misses:    %d\n", cache_stat_inst.misses);
  if (!cache_stat_inst.accesses)
    printf("  miss rate: 0 (0)\n"); 
  else
    printf("  miss rate: %2.4f (hit rate %2.4f)\n", 
	 (float)cache_stat_inst.misses / (float)cache_stat_inst.accesses,
	 1.0 - (float)cache_stat_inst.misses / (float)cache_stat_inst.accesses);
  printf("  replace:   %d\n", cache_stat_inst.replacements);

  printf(" DATA\n");
  printf("  accesses:  %d\n", cache_stat_data.accesses);
  printf("  misses:    %d\n", cache_stat_data.misses);
  if (!cache_stat_data.accesses)
    printf("  miss rate: 0 (0)\n"); 
  else
    printf("  miss rate: %2.4f (hit rate %2.4f)\n", 
	 (float)cache_stat_data.misses / (float)cache_stat_data.accesses,
	 1.0 - (float)cache_stat_data.misses / (float)cache_stat_data.accesses);
  printf("  replace:   %d\n", cache_stat_data.replacements);

  printf(" TRAFFIC (in words)\n");
  printf("  demand fetch:  %d\n", cache_stat_inst.demand_fetches + 
	 cache_stat_data.demand_fetches);
  printf("  copies back:   %d\n", cache_stat_inst.copies_back +
	 cache_stat_data.copies_back);
}
/************************************************************/
