//https://github.com/Denise-VPZ/tpI_bot

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>

#include <time.h>
struct memory {
  char *response;
  size_t size;
};

void cargar_token (char *); 
void def_UPDurl(char *, size_t, char *, long long);
char *conectarT(const char *); 
static size_t cb(char *, size_t, size_t, void *); 
void JSON_parsear_datos(const char *, long *, long long*, char *, char *); 
char *procesar_msj(const char *, const char *); 
int msj_leido(long ); 
void enviar_msj(char*, long long, char *); 
void reg_HsNmMsj(char*, char*);


int main(void){

  printf("Iniciando bot...\n");//linea para chequear, la dejo porque me gusta
  fflush(stdout);

  char token[100];  
  cargar_token (token);
  if (strlen(token) == 0) {
    printf("Error: el token está vacío.\n");
    return 1;
  }

  long long last_update_id = -1;  

  while(1) { 

    char url[500];    
    def_UPDurl(url, sizeof(url), token, last_update_id);

    char *respuesta = conectarT(url);
    if (respuesta == NULL) {
      printf("Error al conectar, reintentando...\n");
      sleep(2);
      continue;
    }

    printf("Respuesta del servidor:\n%s\n", respuesta);//linea para chequear, la dejo porque me gusta 

    long update_id= -1;
    long long chat_id= -1;
    char msj[200];
    char first_name[100];

    JSON_parsear_datos(respuesta, &update_id, &chat_id, msj, first_name);
    
    if (msj_leido(update_id) == 1) { 
      free(respuesta); 
      sleep(2); 
      continue;
    }

    reg_HsNmMsj(first_name, msj);

    char *respuesta_bot = procesar_msj(msj, first_name);
    if (respuesta_bot != NULL) {
      enviar_msj(token, chat_id, respuesta_bot);

      reg_HsNmMsj("Bot TPI", respuesta_bot); 
    }

    if (update_id != -1)
      last_update_id = update_id;
  
    free(respuesta);
    sleep(2);
  }
  return 0;
}

void cargar_token (char *token) {
  FILE * fop = fopen ("token.txt", "r"); 

  if (!fop){ 
    printf ("NO se pudo abrir el archivo token.txt\n");
    return;
  }
  fgets (token,100,fop);
  token[strcspn(token, "\r\n")] = 0;
  fclose(fop); 

}

void def_UPDurl(char *dest, size_t tam, char *token, long long last_id) {
    if (last_id == -1) {
      snprintf(dest, tam,"https://api.telegram.org/bot%s/getUpdates?timeout=10",token);
    } 
    else {
    snprintf(dest, tam,"https://api.telegram.org/bot%s/getUpdates?offset=%lld&timeout=10",token, last_id + 1);
    }
}

static size_t cb(char *data, size_t size, size_t nmemb, void *clientp) {
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

char *conectarT(const char *url) {
    CURL *curl = curl_easy_init(); 
    if (!curl) return NULL; 

    struct memory chunk;
    chunk.response = malloc(1);
    chunk.size = 0;
    if (chunk.response) chunk.response[0] = 0;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_CAINFO, "C:\\\\curl\\\\cacert.pem");

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        free(chunk.response);
        return NULL;
    }

    return chunk.response;  
}

void JSON_parsear_datos(const char *json, long *update_id, long long *chat_id, char *msj, char *first_name) {
  
    if (!json || !update_id || !chat_id  || !msj || !first_name) return; 
    
    char msj_local[200] = "";
    char nombre_local[100] = "";
    long long id_encontrado = -1;
    long update_encontrado = -1;

    char *bloque = strstr(json, "\"message\":");
    if (!bloque) return;

    char *pos_update = strstr(json, "\"update_id\":");
      if (pos_update) {
        pos_update += strlen("\"update_id\":");
        sscanf(pos_update, "%ld", &update_encontrado);
        //printf("Update ID: %ld\n", update_encontrado); //linea para chequear 
    }

    char *pos_text = strstr(bloque, "\"text\":\"");
    if (pos_text) {
        pos_text += strlen("\"text\":\"");
        char *fin = strchr(pos_text, '\"');

        if (fin) {
            int len = (int)(fin - pos_text);
            if (len > 198) len = 198;   
            strncpy(msj_local, pos_text, len);
            msj_local[len] = 0;
            //printf("Mensaje recibido: %s\n", msj_local);//linea para chequear 
        }
    }

    char *pos_chat = strstr(bloque, "\"chat\":{\"id\":");
    if (pos_chat) {
        sscanf(pos_chat, "\"chat\":{\"id\":%lld", &id_encontrado);
        //printf("Chat ID: %lld\n", id_encontrado);//linea para chequear 
    }

    char *pos_name = strstr(bloque, "\"first_name\":\"");
    if (!pos_text)
      printf("No se encontró texto.\n"); 

    if (pos_name) {
        pos_name += strlen("\"first_name\":\"");
        char *fin = strchr(pos_name, '\"');
        if (fin) {
            int len = (int)(fin - pos_name); 
            if (len > 98) len = 98; 
            strncpy(nombre_local, pos_name, len);
            nombre_local[len] = 0;
            //printf("Nombre: %s\n", nombre_local);//linea para chequear 
        }
    }
    
    snprintf(msj, 200, "%s", msj_local);
    snprintf(first_name, 100, "%s", nombre_local);
    *update_id = update_encontrado;
    *chat_id = id_encontrado;

}

char *procesar_msj(const char *msj_recibido, const char *first_name) {
    static char respuesta_buffer[300]; 

    if (strstr(msj_recibido, "hola") != NULL || strstr(msj_recibido, "Hola") != NULL) {
      sprintf(respuesta_buffer, "Hola %s", first_name);
      return respuesta_buffer;
    }

    if (strstr(msj_recibido, "chau") != NULL || strstr(msj_recibido, "Chau") != NULL) { 
      sprintf(respuesta_buffer, "Chau %s, nos vemos. Que tengas buen día.", first_name);
      return respuesta_buffer;
    }

    return NULL;
}

void enviar_msj (char* token, long long chat_id, char* respuesta_bot){ 
  
  CURL *curl = curl_easy_init();
  if (!curl) return;

  char *esc = curl_easy_escape(curl, respuesta_bot, 0);

  char URL[600];
  sprintf(URL,"https://api.telegram.org/bot%s/sendMessage?chat_id=%lld&text=%s", token, chat_id, esc);

  //printf("\n[DEBUG] Intentando enviar a: %s\n", URL);//linea para chequear 

  char *resultado = conectarT(URL);
    
  if (resultado != NULL) {
      //printf("[DEBUG] Respuesta de Telegram al envio: %s\n", resultado);//linea para chequear 
      free(resultado);
  } else {
      printf("[DEBUG] Error: La funcion conectarT devolvio NULL al intentar enviar.\n");
  }

  curl_free(esc);
  curl_easy_cleanup(curl);
}

int msj_leido(long update_id){ 

  static long ultimo_id=-1;

  if (update_id == -1)
      return 1; 
  if (update_id <= ultimo_id)
      return 1; 

  ultimo_id = update_id;
  return 0;

}

void reg_HsNmMsj(char *nombre, char *mensaje) {
    FILE *archivo = fopen("historial.txt", "a"); 
    
    if (archivo == NULL) {
        printf("Error al abrir el historial.\n");
        return;
    }

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    fprintf(archivo, "[%02d:%02d:%02d] %s: %s\n", 
            tm->tm_hour, tm->tm_min, tm->tm_sec, nombre, mensaje);

    fclose(archivo);
}

