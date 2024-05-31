#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
/***
  DECLARACION TIPOS
***/
struct ConfiguracionInicial {
    int usuarios;
    int numEstaciones;
    int huecosPorEstacion;
    int tiempoMinEspera;
    int tiempoMaxEspera;
    int tiempoMinMontando;
    int tiempoMaxMontando;
    int numMinPaseos;
    int numMaxPaseos;
};

struct Ciclista{
    int numero;
    int paseosrealizados;
    int paseostotales;
    int disponible;
    int terminado;
};

struct ThreadArgs {
    int id;
    struct Ciclista *ciclistas;
    struct ConfiguracionInicial *configuracion;
    FILE *fichero;
};
/***
  DECLARACION VARIABLES GLOBALES
***/
//Declaracion de los mutex
pthread_mutex_t  mutex;
//Declaracion de los condition
pthread_cond_t * noLleno;
pthread_cond_t * noVacio;
//Array dinamico de todas las estaciones
int *Estaciones;
//Contadores para controlar la inanicion e interbloqueos
int activos = 0;
int parados = 0;
/***
  DECLARACION FUNCIONES
***/
void *miRutina(void *threadarg);
void MostrarConfig(struct ConfiguracionInicial configuracion);
void EscribirConfig(struct ConfiguracionInicial configuracion,
                    const char *ArchivoSalida);
int * InicializarEstaciones( struct ConfiguracionInicial configuracion);
int inanicion(int maxEstaciones);
int generarAleatorioEnRango(int min, int max);
char * fecha();
struct ConfiguracionInicial leerFichero(const char *nombreArchivo);
struct Ciclista InicializarCiclista(struct Ciclista *ciclistas,
                                    struct ConfiguracionInicial config);
/***
  PROGRAMA PRINCIPAL
***/
int main(int argc, char * argv[]) {
    //Variable del fichero entrada
    char * nEntrada;
    //Variable del fichero salida
    char * nSalida;
    //Variable que almacena el valor de la funcion fecha()
    char * tiempo = fecha();
    //Array de argumentos para los threads
    struct ThreadArgs * thread_args;
    //Variable que guarda la configuracion inicial del fichero de entrada
    struct ConfiguracionInicial configuracion;
    //Array dinamico de todos los usuarios del sistema
    struct Ciclista * ciclistas;
    int n;
    pthread_t * threads;
    //if anidado, comprueba los argumentos de entrada.
    // Asigna el nombre del fichero de entrada y salida
    if (argc < 2){
        nEntrada = strdup("entrada_BiciMAD.txt");
        strcat(tiempo, "_salida_BiciMAD.txt");
        nSalida = strdup(tiempo);
        // Asigna a nEntrada el argumento 1 y crea el de salida
    }else if(argc==2){
        nEntrada = strdup( argv[1]);
        strcat(tiempo, "_salida_BiciMAD.txt");
        nSalida = strdup( tiempo);
        // Asigna el nombre del fichero de entrada y salida con ambos argumentos
    }else{
        nEntrada = strdup( argv[1]);
        nSalida = strdup( argv[2]);
    }
    //Leemos el fichero de configuracion inicial
    configuracion = leerFichero(nEntrada);
    //Se muestra por pantalla la configuracion
    MostrarConfig(configuracion);
    //Se escribe la configuracion en el fichero de salida
    EscribirConfig(configuracion,nSalida);
    //Se inicializan las estaciones con su capacidad
    Estaciones = InicializarEstaciones(configuracion);
    //Se reserva espacio para los argumentos de los hilos
    thread_args = malloc ((configuracion.usuarios + 1) *
                          sizeof (struct ThreadArgs));
    threads = (pthread_t *) malloc ((1 + configuracion.usuarios) *
                                    sizeof(pthread_t));
    //Se reserva espacio para los ususarios
    ciclistas = (struct Ciclista *) malloc(configuracion.usuarios *
                                           sizeof(struct Ciclista));
    *ciclistas = InicializarCiclista(ciclistas,configuracion);
    //Se reserva espacio para los mutex condicionales
    noVacio = (pthread_cond_t *) malloc ((configuracion.numEstaciones +1) *
                                         sizeof(pthread_cond_t));
    noLleno = (pthread_cond_t *) malloc ((configuracion.numEstaciones +1) *
                                         sizeof(pthread_cond_t));
    //Se abre el fichero de salida por el final
    FILE *fichero = fopen(nSalida, "a");
    //Comprobacion de error al abrir el fichero
    if (fichero == NULL) {
        perror("Error al abrir el archivo de salida");
        return 0;
    }
    //Thread condicional para cada estacion (esta llena / esta vacia)
    for(int i=1; i <= configuracion.numEstaciones; i++){
        pthread_cond_init(&noVacio[i], NULL);
        pthread_cond_init(&noLleno[i], NULL);
    }
    //Se inicia el mutex que controlara las secciones críticas
    pthread_mutex_init(&mutex, NULL);
    //Para cada usuario se creara un thread
    for(int i = 0; i<configuracion.usuarios;i++){
        //Argumentos que usará el thread(estaciones, usuario y fichero salida)
        thread_args[i].id = i;
        thread_args[i].configuracion = &configuracion;
        thread_args[i].ciclistas = &ciclistas[i];
        thread_args[i].fichero = fichero;
        ciclistas[i].disponible = 0;
        printf("Empieza el ciclista %d \n", i);
        //Comprobacion de error al crear el thread
        if(pthread_create(&threads[i], NULL,  miRutina,
                          (void *)&thread_args[i])){
            return -1;
        }
        usleep (300000);
        /*Comprobación que pone al usuario en no disponible para
          que no se vuelva a usar en otro thread distinto */
        if( ciclistas[i].paseosrealizados == ciclistas[i].paseostotales) {
            ciclistas[i].terminado = 1;
            ciclistas[i].disponible = 0;
        }else{
            ciclistas[i].disponible = 1;
        }

    }
    //Esperamos a que todos los hilos terminen de ejecutarse
    for(int j = 0; j<configuracion.usuarios;j++){
        pthread_join(threads[j], NULL);
    }
    for(int k = 0; k < configuracion.usuarios;k++){
        printf("ciclista %d : %d paseos\n",k , ciclistas[k].paseostotales);
    }
    //Se notifica el final de todos los hilos
    printf("Todos los ciclistas han terminado.\n");
    //Se destruye el mutex creado
    pthread_mutex_destroy(&mutex);
    //Se destruyen todos los mutex condicionales creados
    for(int k = 1; k <= configuracion.numEstaciones; k++){
        pthread_cond_destroy(&noLleno[k]);
        pthread_cond_destroy(&noVacio[k]);
    }
    //Liberación de memoria dinamica usada
    free(ciclistas);
    free(noVacio);
    free(noLleno);
    free(Estaciones);
    free(tiempo);
    free(threads);
    free (thread_args);
    //Se cierra el fichero de salida
    fclose(fichero);
    return 0;
}
/***
  FUNCION INANICION
***/
int inanicion(int maxEstaciones){
    /*Comprueba si hay usuarios que estan bloqueados esperando en una estacion,
    en ese caso se manda una señal para que despierten del pthread_cond_wait */
    for(int i = 1; i<=maxEstaciones; i++){
        pthread_cond_signal(&noLleno[i]);
        pthread_cond_signal(&noVacio[i]);
    }
}
/***
  FUNCION RUTINA DEL HILO
***/
void *miRutina(void *threadarg) {
    struct ThreadArgs *args = (struct ThreadArgs *) threadarg;
    int num;
    //Se suma un proceso activo
    activos ++;
    //Se realiza un for que completa todos los paseos del usuario
    for(args->ciclistas->paseosrealizados = 1;
        args->ciclistas->paseosrealizados <= args->ciclistas->paseostotales;
        args->ciclistas->paseosrealizados++){
        // Generamos una estación aleatoria
        num = generarAleatorioEnRango(1,
                                      args->configuracion->numEstaciones);
        //El ciclista espera y decide tomar una bici
        usleep(1000000 * generarAleatorioEnRango(
                args->configuracion->tiempoMinEspera,
                args->configuracion->tiempoMaxEspera));
        printf("El ciclista %d quiere tomar en %d.\n", args->id, num);
        fprintf(args->fichero,
            "El ciclista %d quiere tomar en %d.\n", args->id, num);
        //Bloqueamos el mutex por sección crítica
        pthread_mutex_lock(&mutex);
        /*Si la estación esta vacia,
        el usuario esperará a que otra persona deje una bici en la estación*/
        while (Estaciones[num] == 0) {
            printf("El ciclista %d espera para tomar una bici en %d.\n",
                   args->id, num);
            fprintf(args->fichero,
                "El ciclista %d espera a poder tomar una bici en %d.\n",
                    args->id, num);
            parados ++;
            /* Si los unicos procesos activos se encuentran esperando
               buscará una nueva estación */
            if(parados == activos){
                num = generarAleatorioEnRango(1,
                                  args->configuracion->numEstaciones);
                /* Si hay usuarios montando todavía esperará a que alguien deje
                   su bici en esa estación */
            }else{
                pthread_cond_wait(&noVacio[num], &mutex);
            }
            //El usuario ya no está esperando
            parados --;
        }
        //Se saca una bici de la estación
        Estaciones[num]--;
        //Se muestra que el usuario ha cogido su bici
        printf("El ciclista %d toma una bici en %d.\n", args->id, num);
        fprintf(args->fichero,
                "El ciclista %d toma una bici en %d.\n",
                args->id, num);
        //Fin de la sección crítica
        pthread_mutex_unlock(&mutex);
        //Se muetra que el cilista está montando
        printf("El ciclista %d esta montando...\n", args->id);
        fprintf(args->fichero,
                "El ciclista %d esta montando...\n", args->id);
        /*Al liberar un hueco de la estación se notifica a los que
          esten esperando para dejar una bici en esa estación*/
        pthread_cond_signal(&noLleno[num]);
        //tiempo para montar
        usleep(1000000 * generarAleatorioEnRango(
                args->configuracion->tiempoMinMontando,
                args->configuracion->tiempoMaxMontando));
        //Se genera una estación aleatoria para dejar la bici
        num = generarAleatorioEnRango(1,
                                      args->configuracion->numEstaciones);
        //Se muestra quye el usuario quiere dejar la bici
        printf("El ciclista %d quiere dejar una bici en %d.\n",
               args->id, num);
        fprintf(args->fichero,
                "El ciclista %d quiere dejar una bici en %d.\n",
                args->id, num);
        //Se bloquea por sección crítica
        pthread_mutex_lock(&mutex);
        fprintf(args->fichero,
                "El ciclista %d quiere dejar una bici en %d.\n",
                args->id, num);
        //Si la estación esta llena, el usuario espera para dejar la bici
        while (Estaciones[num] == args->configuracion->huecosPorEstacion) {
            parados++;
            //Si todos los usuarios activos están esperando
            if (parados == activos){
                num = generarAleatorioEnRango(1,
                                      args->configuracion->numEstaciones);
                //este usuario busca una nueva estación con hueco para su bici
            }else{
                pthread_cond_wait(&noLleno[num], &mutex);
            }
            //El usuario deja de estar parado
            parados --;
        }
        //Se suma una bici a la estación
        Estaciones[num]++;
        //Se muestra que el usuario ha dejado la bici
        printf("El ciclista %d deja una bici en %d.\n", args->id, num);
        fprintf(args->fichero,
                "El ciclista %d deja una bici en %d.\n", args->id, num);
        //Fin de la sección crítica
        pthread_mutex_unlock(&mutex);
        //Señal a los usuarios que están esperando en la misma estación
        pthread_cond_signal(&noVacio[num]);
    }
    //El usaurio deja de estar activo
    activos --;
    /*Se comprueba si el resto de usuarios activos están esperando y si es asi
     se les despierta ya que nadie les va a dejar una bici o va a coger
     una bici de su estación */
    if(activos != 0){
        if(parados == activos) {
            inanicion(args->configuracion->numEstaciones);
        }
    }
}
/***
  FUNCION GENERAR NUMERO ALEATORIO
***/
//genera un numero aleatorio en un rango especificado
int generarAleatorioEnRango(int min, int max) {
    clock_t c = clock();
    //Utiliza como semilla la hora actual
    srand((unsigned int)c);
    return rand() % (max - min + 1) + min;
}
/***
  FUNCION INICIALIZAR CICLISTA
***/
//Inicialización del array de usuarios
struct Ciclista InicializarCiclista(struct Ciclista *ciclistas,
                                    struct ConfiguracionInicial config){
    for (int i = 0; i< config.usuarios;i++){
        //Numero del usuario
        ciclistas[i].numero = i;
        //Paseos que ya ha realizado
        ciclistas[i].paseosrealizados = 0;
        //Paseos maximos que puede realizar
        ciclistas[i].paseostotales = generarAleatorioEnRango(
                config.numMinPaseos,config.numMaxPaseos);
        //Disponibilidad el usuario
        ciclistas[i].disponible = 1;
        ciclistas[i].terminado = 0;
    }
    return *ciclistas;
}
/***
  FUNCION INICIALIZAR ESTACIONES
***/
int* InicializarEstaciones(struct ConfiguracionInicial configuracion){
    //Se reserva espacio
    int *estaciones = (int *)malloc(configuracion.numEstaciones *
                                    sizeof(int));
    //Se rellenan 3/4 de los huecos de cada estación
    for (int i = 1 ; i<=configuracion.numEstaciones;i++){
        estaciones[i] = configuracion.huecosPorEstacion *3/4;
    }
    return estaciones;
}
/***
  FUNCION FECHA
***/
char* fecha(){
    //Creación de un string con la hora actual
    time_t tiempoActual;
    struct tm *infoTiempo;
    //Se reserva espacio para el buffer
    char * buffer = (char *)malloc(80 * sizeof(char));
    //Se comprueba que no de error
    if (buffer == NULL) {
        perror("Error al asignar memoria para el buffer");
        exit(EXIT_FAILURE);
    }
    time(&tiempoActual);
    //Se obtiene la hora actual
    infoTiempo = localtime(&tiempoActual);
    //Formatear la fecha y hora y se almacena en buffer
    strftime(buffer, 80, "%Y-%m-%d %H-%M-%S",
             infoTiempo);
    return buffer;
}
/***
  FUNCION LEER FICHERO ENTRADA
***/
struct ConfiguracionInicial leerFichero(const char *nombreArchivo) {
    //Struct para guardar la configuracion
    struct ConfiguracionInicial configuracion;
    //Se abre para lectura el fichero de entrada
    FILE *archivo = fopen(nombreArchivo, "r");
    int numero;
    char linea[255];
    char *token;
    //Se notifica el error al usuario
    if (archivo == NULL) {
        perror("Error al abrir el archivo");
        exit(EXIT_FAILURE);
    }
    if (fscanf(archivo, "%d %d %d %d %d %d %d %d %d",
               &configuracion.usuarios,
               &configuracion.numEstaciones,
               &configuracion.huecosPorEstacion,
               &configuracion.tiempoMinEspera,
               &configuracion.tiempoMaxEspera,
               &configuracion.tiempoMinMontando,
               &configuracion.tiempoMaxMontando,
               &configuracion.numMinPaseos,
               &configuracion.numMaxPaseos) != 9) {
        perror("Error al leer la configuración desde el archivo");

        exit(EXIT_FAILURE);
    }
    fclose(archivo);

    return configuracion;
}
/***
  FUNCION ESCRIBIR CONFIGURACION EN CONSOLA
***/
void MostrarConfig(struct ConfiguracionInicial configuracion){
    //Escribe todas las variables del struct configuracion
    printf("CONFIGURACION:\n");
    printf("Usuarios: %d\n", configuracion.usuarios);
    printf("Numero de Estaciones: %d\n",
           configuracion.numEstaciones);
    printf("Huecos por Estacion: %d\n",
           configuracion.huecosPorEstacion);
    printf("Tiempo Minimo de Espera: %d\n",
           configuracion.tiempoMinEspera);
    printf("Tiempo Maximo de Espera: %d\n",
           configuracion.tiempoMaxEspera);
    printf("Tiempo Minimo Montando: %d\n",
           configuracion.tiempoMinMontando);
    printf("Tiempo Maximo Montando: %d\n",
           configuracion.tiempoMaxMontando);
    printf("Numero Minimo de Paseos: %d\n",
           configuracion.numMinPaseos);
    printf("Numero Maximo de Paseos: %d\n",
           configuracion.numMaxPaseos);
    printf("\nSimulacion de funcionamiento de Bicimad\n");
    printf("\n");
}
/***
  FUNCION ESCRIBIR CONFIGURACION EN EL FICHERO SALIDA
***/
void EscribirConfig(struct ConfiguracionInicial configuracion,
                    const char * ArchivoSalida){
    //Se abre para escritura el fichero de salida, si no existe se crea
    FILE *archivoSalida = fopen(ArchivoSalida, "w");
    //En caso de producirse un error se notifica
    if (archivoSalida == NULL) {
        perror("Error al abrir el archivo de salida");
        return;
    }
    //Se escribe en el fichero la configuración inicial
    fprintf(archivoSalida, "CONFIGURACION:\n");
    fprintf(archivoSalida, "Usuarios: %d\n",
            configuracion.usuarios);
    fprintf(archivoSalida, "Numero de Estaciones: %d\n",
            configuracion.numEstaciones);
    fprintf(archivoSalida, "Huecos por Estacion: %d\n",
            configuracion.huecosPorEstacion);
    fprintf(archivoSalida, "Tiempo Minimo de Espera: %d\n",
            configuracion.tiempoMinEspera);
    fprintf(archivoSalida, "Tiempo Maximo de Espera: %d\n",
            configuracion.tiempoMaxEspera);
    fprintf(archivoSalida, "Tiempo Minimo Montando: %d\n",
            configuracion.tiempoMinMontando);
    fprintf(archivoSalida, "Tiempo Maximo Montando: %d\n",
            configuracion.tiempoMaxMontando);
    fprintf(archivoSalida, "Numero Minimo de Paseos: %d\n",
            configuracion.numMinPaseos);
    fprintf(archivoSalida, "Numero Maximo de Paseos: %d\n",
            configuracion.numMaxPaseos);
    fprintf(archivoSalida, "\n");
    //Se cierra el archivo después de escribir
    fclose(archivoSalida);
}