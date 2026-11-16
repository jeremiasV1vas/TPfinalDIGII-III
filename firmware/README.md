# Firmware

```text
firmware/
├── CMSISv2p00_LPC17xx/   # Drivers de la cátedra (LPC17xx-CMSIS-Driver-Enhancement), sin los tests
├── src/                  # Nuestro código (.c)
└── inc/                  # Nuestros headers (.h). Los pines están todos en config.h
```

## Qué hace cada archivo

| Archivo | Qué hace |
|---|---|
| `main.c` | Modos, LED y lo que se hace en cada tick (20 ms) |
| `servos.c` | PWM1 para los 4 servos del esclavo (P2.0 a P2.3) |
| `sensores.c` | ADC en modo burst para los potes y DMA para la corriente de la garra |
| `garra.c` | Garra del esclavo (por ahora, por posición) y su carga medida con el divisor |
| `botones.c` | Botones de grabar (EINT1, P2.11), reproducir (EINT2, P2.12) y referencia de la garra (EINT3, P2.13) |
| `secuencia.c` | Pasos grabados en RAM, hasta 30 segundos |
| `serie.c` | UART2 a 115200 (P0.10 y P0.11) para mandar el estado a la PC |

Los comentarios están en formato Doxygen.

## Modos

| Modo | Qué hace | LED |
|---|---|---|
| Copia | El esclavo sigue al maestro | Rojo parpadeando lento |
| Grabar | Copia y además guarda los pasos | Rojo fijo |
| Reproducir | Repite lo grabado una y otra vez | Verde fijo |

Arranca en *Copia*. Con el botón de grabar se entra y se sale de *Grabar*, y con el de reproducir se entra y se sale de *Reproducir*. Los botones van entre el pin y GND, sin resistencia (se usa el pull-up interno).

## Referencia de la garra

El servo de la garra se alimenta a través de R_SENSE (3 resistencias de 1 Ω en paralelo) y un divisor 3,3k/2,2k lleva su tensión al AD0.5 (P1.31, J2-20). Cuando la garra aprieta algo, el servo pide más corriente y esa lectura baja.

La lectura también se mueve con la fuente de 5 V, así que no sirve sola: hay que compararla con una lectura de referencia tomada con la garra libre. Para eso está el tercer botón, en P2.13 (J2-27):

1. Prender la placa y dejar la garra abierta, sin agarrar nada.
2. Apretar el botón de referencia. En los mensajes de la interfaz aparece `Referencia de la garra: ...` con el valor guardado.
3. Desde ahí, la *carga* que se ve arriba en la interfaz es referencia − lectura. Con la garra libre da cerca de 0, y al apretar algo sube (unos 6 mA por cuenta).

Si se cambia la fuente o se mueve el cableado, conviene volver a tomar la referencia.

## Armar el proyecto en MCUXpresso

Esto lo hace **una sola persona** y sube al repo lo que genera MCUXpresso. Después los demás solo importan.

1. *File > Import > General > Existing Projects into Workspace* y elegir la carpeta del repo. Importar `CMSISv2p00_LPC17xx`.
2. *File > New > Project > C/C++ > New C/C++ Project*, elegir el **LPC1769** y *C Project*.
   - Nombre: `brazos`.
   - Destildar *Use default location* y elegir la carpeta `firmware/` del repo.
   - En *CMSIS Core library* elegir `CMSISv2p00_LPC17xx`.
3. El asistente crea un `src/brazos.c` con su propio `main`: **borrarlo**, porque el nuestro está en `src/main.c`.
4. En *Properties > C/C++ Build > Settings > MCU C Compiler > Includes*, agregar:
   - `"${workspace_loc:/${ProjName}/inc}"`
   - `"${workspace_loc:/CMSISv2p00_LPC17xx/Drivers/inc}"`
5. Compilar y grabar. El LED rojo de la placa tiene que parpadear una vez por segundo.
6. Subir al repo los archivos que generó MCUXpresso (`.project`, `.cproject`, `src/cr_startup_lpc175x_6x.c`, `src/crp.c`). Las carpetas `Debug/` y `Release/` ya están en el `.gitignore`.

Para los que vienen después: *File > Import > Existing Projects into Workspace*, elegir la carpeta del repo e importar los dos proyectos.
