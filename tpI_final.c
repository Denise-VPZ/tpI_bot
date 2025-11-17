#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>

#include <stdbool.h> // Para usar bool
struct memory {
  char *response;
  size_t size;
};

void cargar_token (char *); //abre el archivo Token.txt para leer y guardar en la variable token
void definir_url(char *, char *);  //define la url con el token, que se almacena en el arreglo url
char *conectarT(const char *); //conecta el programa a Telegram (va a la url, pide la info y la trae de vuelta)

static size_t cb(char *, size_t, size_t, void *); //almacena la informacion que trajo conectarT

void JSON_parsear_datos(const char *, long *, char *, char *); 
//analiza y extrae la informacion clave de cb (chat_id, mensaje, first_name) y la guarda

char *procesar_mensaje(const char *, const char *); //hola y chau
char enviar_respuesta(long chat_id, const char *); //
void msj_leido(); // para no responder respetido

//poner todo dentro de un loop

int main(void){
  
  char token[100];  //almacena el token (tendra un tamaño aprox de 50 pero le pongo 100)
  cargar_token (token);

  char url[300];
  definir_url(url, token);

  char *respuesta = conectarT(url);
  if (respuesta == NULL) {
    printf("Error al hacer la petición.\n");
    return 1;
  }
  printf("Respuesta del servidor:\n%s\n", respuesta);
  
  long chat_id;
  char mensaje[200];
  char first_name[100];
  JSON_parsear_datos(respuesta, &chat_id, mensaje, first_name);

  char *respuesta_bot = procesar_mensaje(mensaje, first_name); //revisar

  
  free(respuesta);// LIBERAR MEMORIA

}

void cargar_token (char *token) {
  FILE * fop = fopen ("Token.txt", "r"); 
  if (fop == NULL){
    printf ("NO se pudo abrir el archivo %s", token); //revisar %s y token
    return;
  }
  /*fgets(destino, tamaño_max, archivo_origen); usado para leer cadenas de texto
  Lee una línea completa de texto DESDE el archivo 
  La guarda en el arreglo destino
  Detiene la lectura cuando aparece: un salto de línea \n, el final del archivo o cuando se llenó el buffer
  Siempre agrega automáticamente el terminador \0*/
  // lee el token
  fgets (token,100,fop);//Leer hasta 99 caracteres del archivo f y guardarlos dentro del arreglo token.

  fclose(fop); //cierra el archivo

}

void definir_url(char *url, char *token){
  sprintf(url, "https://api.telegram.org/bot%s/getUpdates", token);
}

static size_t cb(char *data, size_t size, size_t nmemb, void *clientp)
{
  size_t realsize = nmemb;
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

void JSON_parsear_datos(const char *json, long *chat_id, char *mensaje, char *first_name) {
  
  //evitar segmentation faults (fallos de segmentación) si se llama a la función con algún puntero nulo.
    if (!json || !chat_id || !mensaje || !first_name) return; 
    
  //extraer los datos primero en un búfer seguro y luego copiarlo a la variable de destino al final
    char mensaje_local[200] = "";
    char nombre_local[100] = "";
    long id_encontrado = -1;

    // BUSCAR EL TEXTO DEL MENSAJE
    char *pos_text = strstr(json, "\"text\":\"");
    if (pos_text) {
        pos_text += strlen("\"text\":\"");
        char *fin = strchr(pos_text, '\"');

        if (fin) {
            int len = (int)(fin - pos_text);
            
            //Manejo Seguro de Longitud (Overflow)
            if (len > (int)sizeof(mensaje_local) - 1) len = (int)sizeof(mensaje_local) - 1;
            strncpy(mensaje_local, pos_text, len);
            mensaje_local[len] = '\0';
            printf("Mensaje recibido: %s\n", mensaje_local);
        }
    }

    // BUSCAR EL CHAT ID
    char *pos_chat = strstr(json, "\"chat\":{\"id\":");
    if (pos_chat) {
        // sscanf devuelve el nº de items leídos; no hace falta comprobar aquí si falla,
        // id_encontrado queda -1 si no se encontró
        sscanf(pos_chat, "\"chat\":{\"id\":%ld", &id_encontrado);
        printf("Chat ID: %ld\n", id_encontrado);
    }

    // BUSCAR FIRST_NAME
    // Nota: first_name puede aparecer en "from" o en "chat". Esta búsqueda toma la primera aparición.
    char *pos_name = strstr(json, "\"first_name\":\"");
    if (pos_name) {
        pos_name += strlen("\"first_name\":\"");
        char *fin = strchr(pos_name, '\"');

        if (fin) {
            int len = (int)(fin - pos_name);
            if (len > (int)sizeof(nombre_local) - 1) len = (int)sizeof(nombre_local) - 1;
            strncpy(nombre_local, pos_name, len);
            nombre_local[len] = '\0';
            printf("Nombre: %s\n", nombre_local);
        }
    }

    // GUARDAR RESULTADOS EN LAS VARIABLES DE SALIDA (con seguridad para no overflow)
    snprintf(mensaje, 200, "%s", mensaje_local);
    snprintf(first_name, 100, "%s", nombre_local);
    *chat_id = id_encontrado;


}

/*@brief Procesa el mensaje recibido y genera el texto de respuesta (basado en 'hola' y 'chau').
@param mensaje_recibido La cadena de texto extraída por JSON_parsear_datos.
@return bool Retorna 'true' si se encontró una coincidencia (hola/chau) y 'false' en otro caso.*/
// NOTA: Se necesita modificar la función para que reciba también el nombre del usuario 
// y devuelva la respuesta (respuesta_generada) para cumplir con el punto 4.
char *procesar_mensaje(const char *mensaje_recibido, const char *first_name) {
    static char respuesta_buffer[300]; 
    
    // Convertir el mensaje a minúsculas para una detección más robusta
    // (¡Sería una mejora importante a considerar!)
    
    // 1. Mensaje contiene "hola" (o alguna variación de saludo)
    if (strstr(mensaje_recibido, "hola") != NULL) {
        // Requisito 4: "Hola, <usuario>"
        sprintf(respuesta_buffer, "Hola, %s!", first_name);
        return respuesta_buffer;
    }

    // 2. Mensaje contiene "chau" (despedida)
    if (strstr(mensaje_recibido, "chau") != NULL) {
        // Requisito 5: Saludo apropiado.
        return "¡Hasta luego! Que tengas un excelente día. 👋";
    }

    // 3. Respuesta por defecto (si no coincide con 'hola' o 'chau')
    return "Solo puedo entender 'hola' o 'chau'. ¿Podrías intentar con alguno de esos? 🤔";
}// Para que esto funcione, ¡recuerda que tu función JSON_parsear_datos también debe extraer el 'first_name'!


/*
API es...
Existen muchos servidores que proveen información usando JSON...
 
Trabajo : implementación de un bot para la aplicación de mensajería Telegram.
Bot es... token es...
Si se cuenta con un token (Ej 1234567890:EstoEsUnTokenFalsoSoloParaPruebasss) y se 
se ingresa en el navegador web la URL: 
https://api.telegram.org/bot1234567890:EstoEsUnTokenFalsoSoloParaPruebasss/getUpdates 
se recibirá algo como 
 }"ok": true, 
  "result": [ 
  { "update_id": 746602377, 
    "message": { 
      "message_id": 1005, 
      "from": { 
        "id": 100000000, 
        "is_bot": false, 
        "first_name": "Claudio", 
        "last_name": "Paz", 
        "username": "claudiojpaz", 
        "language_code": "es"       }, 
      "chat": { 
        "id": 100000000, 
        "first_name": "Claudio", 
        "last_name": "Paz", 
        "username": "claudiojpaz", 
        "type": "private"           }, 
      "date": 1762883458, 
      "text": "Prueba de API"             } 
  }         ]       } 
La URL para chequear si existen mensajes debe ser: 
https://api.telegram.org/bot<TOKEN>/getUpdates 
Donde <TOKEN> es el código entregado por el botFather. Si no hay mensajes el JSON se verá como:
{ "ok": true, "result": [] } 

En caso de errores se verá algo como 
{ "ok": false, "error_code": 401, "description": "Unauthorized" } 
donde el error_code indicará cuál fue justamente el error. 

Cada vez que se haga la petición  https://api.telegram.org/bot<TOKEN>/getUpdates  
el bot contestará lo mismo si ya había un mensaje, o sea, seguirá devolviendo el JSON 
correspondiente a ese mensaje. Se puede identificar cada mensaje con el campo  update_id 
de la respuesta. Para indicar al servidor que el mensaje ya fue leído, se debe usar la URL 
https://api.telegram.org/bot<TOKEN>/getUpdates?offset=<update_id+1> 
donde luego de offset= hay que colocar el entero correspondiente al  update_id  pero 
incrementado en por lo menos una unidad. En el ejemplo anterior del mensaje de prueba el id 
era  746602377  entonces para indicarle al servidor que ya se leyó el mensaje, se le pide el 
siguiente: 
https://api.telegram.org/bot<TOKEN>/getUpdates?offset=746602378 
el resultado (si no hay nuevo mensaje) sería nuevamente 
{ "ok": true, "result": [] } 

a partir de este punto se puede seguir preguntando por el 746602378 hasta que aparezca y 
entonces incrementarlo, o simplemente hacer la petición con 
https://api.telegram.org/bot<TOKEN>/getUpdates 
hasta que no haya mensajes. 

Para enviar un mensaje al usuario hay que identificar el campo  chat  el cuál tiene una 
subestructura donde hay que buscar el campo  id.  En el JSON que en el ejemplo es 
100000000  (número ficticio). Con este número hay que usar el método /sendMessage de la API 
https://api.telegram.org/bot<TOKEN>/sendMessage?chat_id=100000000&text=Hola 
Si el mensaje es una oración de más de una palabra, los espacios deben ser reemplazados por 
la secuencia %20 ya que las URLs no pueden contener espacios en blanco y necesitan ser codificadas 
para garantizar que se interpreten correctamente por los servidores web y navegadores. 
Este proceso, llamado codificación de URL, usa una secuencia de porcentaje y dígitos hexadecimales 
para representar caracteres especiales. En el caso del espacio, su representación es %20. 
Otros caracteres especiales, como # o ?, también se codifican con una secuencia similar, 
por ejemplo, %23 y %3F respectivamente. Si el mensaje se envía de manera correcta, 
el resultado que se verá en el navegador será otra cadena JSON con los datos del mensaje como la siguiente: 
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
2)  Si se encuentra un mensaje nuevo, se debe procesar para extraer el update_id para 
usarlo en la siguiente petición de manera que no se repitan los mensajes. 
3)  Se debe procesar el mensaje para extraer el id del chat de manera que la respuesta 
(cuando se use el método /sendMessage) llegue a quien inició la conversación 
4)  Si el mensaje contiene “hola” el bot debe saludar con “Hola, <usuario>” donde 
<usuario> debe ser el nombre de quien inició la conversación. (usar campo 
first_name  de la subestructura  chat  de la cadena JSON recibida  ) 
5)  Si el mensaje contiene “chau” el bot debe saludar con un saludo apropiado. 
6)  Se debe crear un repositorio público en  github.com  o  gitlab.com  donde se debe subir el 
archivo del bot. Además la URL del proyecto debe figurar como comentario en las 
primeras líneas del archivo .c 
7)  Por cuestiones de seguridad, como se trata de un repositorio público, el token del bot 
no debe estar en el repositorio. Se debe leer desde un archivo cuyo nombre y ruta se 
debe pasar como argumento en el llamado del programa. Si no se pasa el nombre de 
archivo, el programa debe finalizar con un aviso. 
8)  Cada mensaje enviado o recibido por el bot debe quedar registrado en un archivo de 
texto registrando: hora (se puede usar el tiempo unix del JSON), nombre, mensaje. 
Se adjunta el programa  base  tpI_base.c  y la guía botFather.pdf para generar el bot. 
Se debe entregar en la UV un archivo en lenguaje C llamado  tpI_final.c  antes de la fecha 
de cierre de la actividad. En las primeras líneas del programa debe aparecer la URL del 
repositorio público donde se pueda encontrar el bot. No se debe subir a la UV o al repositorio el 
token del bot.*/