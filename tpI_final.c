#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>

struct memory {
  char *response;
  size_t size;
};

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

int main(void)
{
  char *api_url = "https://jsonplaceholder.typicode.com/todos/1";

  CURLcode res;
  CURL *curl = curl_easy_init();
  struct memory chunk = {0};

  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, api_url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    //las dos lineas siguientes son para que me ande el curl en Vscode
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_CAINFO, "C:\\\\curl\\\\cacert.pem");

    res = curl_easy_perform(curl);
    if (res != 0)
      printf("Error Código: %d\n", res);

    printf("%s\n", chunk.response);

    free(chunk.response);
    curl_easy_cleanup(curl);
  }
}

/*
API: Application Programming Interface (Interfaz de Programación de Aplicaciones). 
API es: un conjunto de reglas que permite que dos programas distintos se comuniquen entre sí. 
Al usar API: un programa no necesita saber cómo funciona internamente el otro sistema, 
solo necesita enviar pedidos en el formato correcto y leer las respuestas. 
 
Existen muchos servidores que proveen información usando JSON, también hay muchas web 
que devuelven cadenas de prueba, como  https://jsonplaceholder.typicode.com/todos/1 
En un navegador web se puede probar esa URL y devolverá 
{ "userId": 1, "id": 1, "title": "delectus aut autem", "completed": false } 
 
Trabajo : implementación de un bot para la aplicación de mensajería Telegram.
Un bot es un programa que se comporta como un usuario más dentro de la aplicación, 
pero que responde de manera automática a los mensajes que recibe. No usa la aplicación directamente, 
sino que se comunica con los servidores de Telegram a través de su API. 
Cada bot tiene un token de acceso (Ver documento botFather.pdf adjunto) que lo identifica ante 
la plataforma. Mediante ese token, un programa externo puede consultar los mensajes 
recibidos por el bot, enviar respuestas, y realizar otras acciones como enviar fotos, documentos 
o desplegar botones interactivos.
Si ya se cuenta con un token (Ej 1234567890:EstoEsUnTokenFalsoSoloParaPruebasss) y se 
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
La URL para chequear si existen mensajes debe ser: https://api.telegram.org/bot<TOKEN>/getUpdates 
Donde <TOKEN> es el código entregado por el botFather. 
Si no hay mensajes el JSON se verá como 
{ "ok": true, "result": [] } 

En caso de errores se verá algo como 
{ "ok": false, "error_code": 401, "description": "Unauthorized" } 
donde el error_code indicará cuál fue justamente el error. 

Cada vez que se haga la petición  https://api.telegram.org/bot<TOKEN>/getUpdates  el bot 
contestará lo mismo si ya había un mensaje, o sea, seguirá devolviendo el JSON 
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