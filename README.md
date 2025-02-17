# Simulador de Bicicletas Compartidas

![Language](https://img.shields.io/badge/language-C-blue)  
![Status](https://img.shields.io/badge/status-Finished-green)

## Descripción
Este proyecto es un simulador de un sistema de bicicletas compartidas similar a BiciMAD. Implementado en C, utiliza múltiples hilos (`pthread`) para simular ciclistas que toman y dejan bicicletas en estaciones, respetando condiciones de concurrencia mediante `mutex` y variables de condición.

## Características
- Simulación de usuarios (ciclistas) con diferentes comportamientos.
- Implementación de estaciones de bicicletas con capacidad limitada.
- Control de concurrencia mediante `pthread_mutex` y `pthread_cond`.
- Configuración personalizable mediante un archivo de entrada.
- Registro de eventos en un archivo de salida.

## Instalación
1. Clonar este repositorio:
   ```sh
   git clone https://github.com/tu_usuario/simulador-bicimad.git
   cd simulador-bicimad
   ```
2. Compilar el programa:
   ```sh
   gcc -o simulador simulador.c -lpthread
   ```
3. Ejecutar la simulación:
   ```sh
   ./simulador [archivo_entrada] [archivo_salida]
   ```
   - Si no se especifican archivos, se usa `entrada_BiciMAD.txt` por defecto.

## Uso
1. Crear un archivo de configuración (`entrada_BiciMAD.txt`) con el siguiente formato:
   ```txt
   <usuarios> <numEstaciones> <huecosPorEstacion> <tiempoMinEspera> <tiempoMaxEspera> <tiempoMinMontando> <tiempoMaxMontando> <numMinPaseos> <numMaxPaseos>
   ```
   Ejemplo:
   ```txt
   10 5 10 1 3 2 5 1 3
   ```
2. Ejecutar el simulador y analizar el archivo de salida.

## Funcionalidades clave
- **Manejo de concurrencia:** Control mediante `mutex` y `condición` para evitar inanición e interbloqueos.
- **Registro de actividad:** Genera un archivo de salida con detalles de la simulación.
- **Configuración flexible:** Permite ajustar el número de ciclistas, estaciones y tiempos de espera.

## Contribución
1. Haz un fork del repositorio.
2. Crea una nueva rama (`git checkout -b feature-nueva-funcionalidad`).
3. Realiza tus cambios y súbelos (`git commit -m 'Añadir nueva funcionalidad'`).
4. Sube la rama (`git push origin feature-nueva-funcionalidad`).
5. Abre un pull request.

## Licencia
Este proyecto está bajo la Licencia MIT. Consulta el archivo `LICENSE` para más detalles.

