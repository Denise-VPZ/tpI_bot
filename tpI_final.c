//https://github.com/Denise-VPZ/tpI_bot

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
struct memory {
  char *response;
  size_t size;
};

void cargar_token (char *); //abre el archivo Token.txt para leer y guardar en la variable token
void def_UPDurl(char *, size_t, char *, long);
char *conectarT(const char *); //conecta el programa a Telegram (va a la url, pide la info y la trae de vuelta)

static size_t cb(char *, size_t, size_t, void *); //almacena la informacion que trajo conectarT

void JSON_parsear_datos(const char *, long *, long*, char *, char *); 
//analiza y extrae la informacion clave de cb (chat_id, update_id, msj, first_name) y la guarda

char *procesar_msj(const char *, const char *); //hola y chau

int msj_leido(long ); // para no responder respetido
void enviar_msj(char*, long, char *); //envia la respuesta del bot


int main(void){
  //cargar_token hasta definir la url se hacen una vez asi que van fuera del loop
  char token[100];  //almacena el token (tendra un tamaño aprox de 50 pero le pongo 100)
  cargar_token (token);
  if (strlen(token) == 0) {
    printf("Error: el token está vacío.\n");
    return 1;
  }


  long last_update_id = -1;  // Mantener el ID del último mensaje procesado

  while(1) { //loop

    //construiur url del get updates
    char url[500];    //almacena el url (tendra un tamaño aprox de ... pero le pongo 500)
    //define la url con el token, que se almacena en el arreglo url
    def_UPDurl(url, sizeof(url), token, last_update_id);
    
    //conectar y traer la informmacion
    char *respuesta = conectarT(url);

    //verificacion de respuesta
    if (respuesta == NULL) {
      printf("Error al conectar, reintentando...\n");
      sleep(2);// evitar que el loop corra demasiado rápido en error
      continue;
    }

    // si no hay texto, no hay mensaje para procesar
    if (!strstr(respuesta, "\"text\":\"")) {
      free(respuesta);
      sleep(2);
      continue;
    }

    //printf("Respuesta del servidor:\n%s\n", respuesta);
    long update_id= -1;
    long chat_id= -1;
    char msj[200];
    char first_name[100];

    JSON_parsear_datos(respuesta, &update_id, &chat_id, msj, first_name);
    
    if (msj_leido(update_id) == 1) { // evita mensajes repetidos o inválidos   
      free(respuesta); //se libera ya que el continue saltara al proximo ciclo
      sleep(2); // evitar que el loop corra demasiado rápido en error
      continue;
    }
    // procesar el mensaje
    char *respuesta_bot = procesar_msj(msj, first_name);//revisar
    if (respuesta_bot != NULL) 
      enviar_msj(token, chat_id, respuesta_bot);

    // ACTUALIZAR UPDATE_ID
    if (update_id != -1)
      last_update_id = update_id;
  
    free(respuesta);// LIBERAR MEMORIA
    sleep(2);//consulta info cada 2 segundos
  }

  return 0;
}


void cargar_token (char *token) {
  FILE * fop = fopen ("Token.txt", "r"); 

  if (!fop){ //==NULL
    printf ("NO se pudo abrir el archivo Token.txt\n");
    return;
  }
  /*fgets(destino, tamaño_max, archivo_origen); usado para leer cadenas de texto
  Lee una línea completa de texto DESDE el archivo 
  La guarda en el arreglo destino
  Detiene la lectura cuando aparece: un salto de línea \n, el final del archivo o cuando se llenó el buffer
  Siempre agrega automáticamente el terminador \0*/
  // lee el token
  fgets (token,100,fop);//Leer hasta 99 caracteres del archivo f y guardarlos dentro del arreglo token.
  token[strcspn(token, "\r\n")] = 0;// Eliminar salto de línea si existe

  fclose(fop); //cierra el archivo

}

void def_UPDurl(char *dest, size_t tam, char *token, long last_id) {
    snprintf(dest, tam,
        "https://api.telegram.org/bot%s/getUpdates?offset=%ld",
        token, last_id + 1);
}

static size_t cb(char *data, size_t size, size_t nmemb, void *clientp)
{
  size_t realsize = size * nmemb;
  struct memory *mem = clientp;

  char *ptr = realloc(mem->response, mem->size + realsize + 1);
  if(!ptr)
    return 0;  /* out of memory */

  mem->response = ptr;
  memcpy(&(mem->response[mem->size]), data, realsize);
  mem->size += realsize;
  mem->response[mem->size] = 0;

  return realsize;
}

char *conectarT(const char *url)
{
    CURL *curl = curl_easy_init(); // inicia libcurl
    if (!curl) return NULL; //verfica inicializacion de libcurl, !=curl es curl == NULL

    struct memory chunk;
    chunk.response = malloc(1);
    chunk.size = 0;
    if (chunk.response) chunk.response[0] = 0;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    //las dos lineas siguientes son para que me ande el curl en Vscode
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_CAINFO, "C:\\\\curl\\\\cacert.pem");

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        free(chunk.response);
        return NULL;
    }

    return chunk.response;  // EL QUE LLAMA DEBE HACER free()
}

void JSON_parsear_datos(const char *json, long *update_id, long *chat_id, char *msj, char *first_name) {
  
  //evitar segmentation faults (fallos de segmentación) si se llama a la función con algún puntero nulo.
    if (!json || !update_id || !chat_id  || !msj || !first_name) return; //si alguno es NULL volver
    
  //extraer los datos primero en un búfer seguro y luego copiarlo a la variable de destino al final
    char msj_local[200] = "";
    char nombre_local[100] = "";
    long id_encontrado = -1;
    long update_encontrado = -1;

    // BUSCAR EL UODATE_ID
    char *pos_update = strstr(json, "\"update_id\":");
      if (pos_update) {
        // sscanf devuelve el nº de items leídos; no hace falta comprobar aquí si falla,
        // update_encontrado queda -1 si no se encontró
        pos_update += strlen("\"update_id\":");
        sscanf(pos_update, "%ld", &update_encontrado);
        //printf("Update ID: %ld\n", update_encontrado);
    }

    // BUSCAR EL TEXTO DEL MENSAJE
    char *pos_text = strstr(json, "\"text\":\"");
    if (pos_text) {
        pos_text += strlen("\"text\":\"");
        char *fin = strchr(pos_text, '\"');

        if (fin) {
            int len = (int)(fin - pos_text);
            
            //Manejo Seguro de Longitud (Overflow)
            if (len > (int)sizeof(msj_local) - 1) len = (int)sizeof(msj_local) - 1;
            strncpy(msj_local, pos_text, len);
            msj_local[len] = '\0';
            //printf("Mensaje recibido: %s\n", msj_local);
        }
    }

    // BUSCAR EL CHAT ID
    char *pos_chat = strstr(json, "\"chat\":{\"id\":");
    if (pos_chat) {
        // sscanf devuelve el nº de items leídos; no hace falta comprobar aquí si falla,
        // id_encontrado queda -1 si no se encontró
        sscanf(pos_chat, "\"chat\":{\"id\":%ld", &id_encontrado);
        //printf("Chat ID: %ld\n", id_encontrado);
    }

    // BUSCAR FIRST_NAME
    // Nota: first_name puede aparecer en "from" o en "chat". Esta búsqueda toma la primera aparición.
    char *pos_name = strstr(json, "\"first_name\":\"");

    if (!pos_text) //comprobacion
      printf("No se encontró texto.\n"); 

    if (pos_name) {
        pos_name += strlen("\"first_name\":\"");
        char *fin = strchr(pos_name, '\"');

        if (fin) {
            int len = (int)(fin - pos_name);
            if (len > (int)sizeof(nombre_local) - 1) len = (int)sizeof(nombre_local) - 1;
            strncpy(nombre_local, pos_name, len);
            nombre_local[len] = '\0';
            //printf("Nombre: %s\n", nombre_local);
        }
    }
    
    // GUARDAR RESULTADOS EN LAS VARIABLES DE SALIDA (con seguridad para no overflow)
    snprintf(msj, 200, "%s", msj_local);
    snprintf(first_name, 100, "%s", nombre_local);
    *update_id = update_encontrado;
    *chat_id = id_encontrado;

}

/*@brief Procesa el msj recibido y genera el texto de respuesta (basado en 'hola' y 'chau').
@param msj_recibido La cadena de texto extraída por JSON_parsear_datos.
@return bool Retorna 'true' si se encontró una coincidencia (hola/chau) y 'false' en otro caso. 
NOTA: Se necesita modificar la función para que reciba también el nombre del usuario y devuelva 
la respuesta (respuesta_generada) para cumplir con el punto 4.*/
char *procesar_msj(const char *msj_recibido, const char *first_name) {
    static char respuesta_buffer[300]; 

    // Convertir el msj a minúsculas para una detección más robusta
    // (¡Sería una mejora importante a considerar!)
    
    // 1. Mensaje contiene "hola" (o alguna variación de saludo)
    if (strstr(msj_recibido, "hola") != NULL) {
      sprintf(respuesta_buffer, "Hola %s", first_name);// sin %20 pq url_easy_escape() YA codifica los espacios.
      return respuesta_buffer;
    }
    // 2. Mensaje contiene "chau" (despedida)
    if (strstr(msj_recibido, "chau") != NULL) { 
      sprintf(respuesta_buffer, "Chau %s, nos vemos. Que tengas buen día.", first_name);
      return respuesta_buffer;
    }

    return NULL;
}

void enviar_msj (char* token, long chat_id, char* respuesta_bot){
 //en chat_id ya tiene el valor del puntero asi que directmente lo usa no necesita ser*
  //url para enviar msj

  CURL *curl = curl_easy_init();
  if (!curl) return;

  // CODIFICAR el mensaje para URL
  char *esc = curl_easy_escape(curl, respuesta_bot, 0);

  char URL[600];
  sprintf(URL,"https://api.telegram.org/bot%s/sendMessage?chat_id=%ld&text=%s", token, chat_id, esc);


  // Reutilizamos función conectarT para mandar la petición // enviar
  char *resultado = conectarT(URL);
    
  // Limpiamos la memoria que devuelve conectarT 
  if(resultado != NULL) free(resultado);

  curl_free(esc);
  curl_easy_cleanup(curl);

}

int msj_leido(long update_id){ //comprueba si el mensaje ya se leyo antes

  static long ultimo_id=-1;

  if (update_id == -1)
      return 1; // no hay mensaje válido

  if (update_id <= ultimo_id)
      return 1; // mensaje viejo o repetido

  // mensaje nuevo
  ultimo_id = update_id;
  return 0;

}


/*

Trabajo : implementación de un bot para la aplicación de mensajería Telegram.

La URL para chequear si existen mensajes debe ser: https://api.telegram.org/bot<TOKEN>/getUpdates  

Cada vez que se haga la petición  https://api.telegram.org/bot<TOKEN>/getUpdates  
el bot contestará lo mismo si ya había un msj, o sea, seguirá devolviendo el JSON correspondiente a ese msj. 
Se puede identificar cada msj con el campo  update_id de la respuesta.

Para indicar al servidor que el msj ya fue leído, se debe usar la URL 
https://api.telegram.org/bot<TOKEN>/getUpdates?offset=<update_id+1> 

donde luego de offset= hay que colocar el entero correspondiente al  update_id  pero 
incrementado en por lo menos una unidad. 

ej: el msj de prueba el id:746602377  entonces para indicarle al servidor que ya se leyó el msj, 
se le pide el siguiente: https://api.telegram.org/bot<TOKEN>/getUpdates?offset=746602378 
el resultado (si no hay nuevo msj) sería nuevamente: { "ok": true, "result": [] } 
a partir de este punto se puede seguir preguntando por el 746602378 hasta que aparezca y 
entonces incrementarlo, o simplemente hacer la petición con https://api.telegram.org/bot<TOKEN>/getUpdates 
hasta que no haya mensajes. 

Para enviar un msj al usuario hay que identificar el campo  chat  el cuál tiene una 
subestructura donde hay que buscar el campo  id.  

En el JSON que en el ejemplo es 100000000  (número ficticio). Con este número hay que usar 
el método /sendMessage de la API https://api.telegram.org/bot<TOKEN>/sendMessage?chat_id=100000000&text=Hola 

Si el msj es una oración de más de una palabra, los espacios deben ser reemplazados por 
la secuencia %20 ya que las URLs no pueden contener espacios en blanco y necesitan ser codificadas 
para garantizar que se interpreten correctamente por los servidores web y navegadores. 
Este proceso, llamado codificación de URL, usa una secuencia de porcentaje y dígitos hexadecimales 
para representar caracteres especiales. En el caso del espacio, su representación es %20. 
Otros caracteres especiales, como # o ?, también se codifican con una secuencia similar, 
por ejemplo, %23 y %3F respectivamente. Si el msj se envía de manera correcta, 
el resultado que se verá en el navegador será otra cadena JSON con los datos del msj como la siguiente: 
 { 
    "ok": true, 
    "result": { 
      "message_id": 1008, 
      "from": { 
        "id": 100000001, 
        "is_bot": true, 
        "first_name": "react_sci", 
        "username": "react_sci_bot"   }, 
      "chat": { 
        "id": 100000000, 
        "first_name": "Claudio", 
        "last_name": "Paz", 
        "username": "claudiojpaz", 
        "type": "private"             }, 
      "date": 1762886599, 
      "text": "Hola"                  } 
  } 

El objetivo de este trabajo es desarrollar un bot de Telegram sencillo escrito en lenguaje C que 
responda automáticamente cuando recibe un saludo (por ejemplo, “hola”, “buen día”, etc.).


Para realizarlo se debe utilizar la biblioteca libcurl, aprovechando el código propuesto. 
1)  Implementar un bot de Telegram que una vez iniciado su programa quede en un loop 
infinito consultando el método /getUpdates cada 2 segundos (usar la función sleep(2); 
disponible por incluir  unistd.h  ) 
2)  Si se encuentra un msj nuevo, se debe procesar para extraer el update_id para 
usarlo en la siguiente petición de manera que no se repitan los mensajes. 
3)  Se debe procesar el msj para extraer el id del chat de manera que la respuesta 
(cuando se use el método /sendMessage) llegue a quien inició la conversación 
4)  Si el msj contiene “hola” el bot debe saludar con “Hola, <usuario>” donde 
<usuario> debe ser el nombre de quien inició la conversación. (usar campo 
first_name  de la subestructura  chat  de la cadena JSON recibida  ) 
5)  Si el msj contiene “chau” el bot debe saludar con un saludo apropiado. 
6)  Se debe crear un repositorio público en  github.com  o  gitlab.com  donde se debe subir el 
archivo del bot. Además la URL del proyecto debe figurar como comentario en las 
primeras líneas del archivo .c 
7)  Por cuestiones de seguridad, como se trata de un repositorio público, el token del bot 
no debe estar en el repositorio. Se debe leer desde un archivo cuyo nombre y ruta se 
debe pasar como argumento en el llamado del programa. Si no se pasa el nombre de 
archivo, el programa debe finalizar con un aviso. 
8)  Cada msj enviado o recibido por el bot debe quedar registrado en un archivo de 
texto registrando: hora (se puede usar el tiempo unix del JSON), nombre, msj. 
Se adjunta el programa  base  tpI_base.c  y la guía botFather.pdf para generar el bot. 
Se debe entregar en la UV un archivo en lenguaje C llamado  tpI_final.c  antes de la fecha 
de cierre de la actividad. En las primeras líneas del programa debe aparecer la URL del 
repositorio público donde se pueda encontrar el bot. No se debe subir a la UV o al repositorio el 
token del bot.*/