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
| `main.c` | Modos, LED, comandos de la PC y lo que se hace en cada tick (20 ms) |
| `servos.c` | PWM1 para los 4 servos del esclavo (P2.0 a P2.3) |
| `sensores.c` | ADC en modo burst para los potes y DMA para la corriente de la garra |
| `garra.c` | Corriente a fuerza, y garra por posición o con el PID de fuerza |
| `proteccion.c` | Umbral de sobrecorriente con el DAC (P0.26) y aviso por EINT3 (P2.13) |
| `botones.c` | Botones de grabar (EINT1, P2.11) y reproducir (EINT2, P2.12) |
| `secuencia.c` | Pasos grabados en RAM, hasta 30 segundos |
| `serie.c` | UART2 con la PC (TX P0.10, RX P0.11), 115200 8N1 |

Los comentarios están en formato Doxygen.

## Modos

| Modo | Qué hace | LED |
|---|---|---|
| 0 Copia | El esclavo sigue al maestro | Parpadeo lento |
| 1 Grabar | Copia y además guarda los pasos | Prendido |
| 2 Reproducir | Repite lo grabado una y otra vez | Parpadeo rápido |
| 3 Manual | Los servos los mueve la PC (calibrar y caracterizar) | Apagado |
| 4 Falla | Saltó la sobrecorriente: garra abierta y todo quieto | Parpadeo muy rápido |

Con el botón de grabar se entra y se sale de *Grabar* (y se sale de *Falla*). Con el de reproducir se entra y se sale de *Reproducir*.

## Mensajes con la PC

Todo es texto, una línea por mensaje. Se puede probar con cualquier terminal serie.

La placa manda 25 veces por segundo:

```text
$T,modo,fuerza_on,pote_base,pote_hombro,pote_codo,pote_garra,pulso_base,pulso_hombro,pulso_codo,pulso_garra,referencia_g,fuerza_g,corriente_mA,umbral_mA,pasos
```

y `$FALLA` cuando salta la protección. Los comandos que entiende son estos, y a cada uno contesta `$OK,...` o `$ERROR,...`:

| Comando | Qué hace |
|---|---|
| `M,n` | Cambia al modo n (0 a 3) |
| `F,1` / `F,0` | Garra con PID de fuerza / garra por posición |
| `K,kp,ki,kd` | Ganancias del PID multiplicadas por 1000 (`K,500,5000,0` es 0,5; 5; 0) |
| `U,mA` | Corriente a la que salta la protección |
| `S,servo,pulso` | Mueve un servo (1 base, 2 hombro, 3 codo, 4 garra) a ese pulso en µs. Solo en modo manual |

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
