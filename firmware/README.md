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
| `garra.c` | Garra del esclavo (por ahora, por posición) |
| `botones.c` | Botones de grabar (EINT1, P2.11) y reproducir (EINT2, P2.12) |
| `secuencia.c` | Pasos grabados en RAM, hasta 30 segundos |

Los comentarios están en formato Doxygen.

## Modos

| Modo | Qué hace | LED |
|---|---|---|
| Copia | El esclavo sigue al maestro | Parpadeo lento |
| Grabar | Copia y además guarda los pasos | Prendido |
| Reproducir | Repite lo grabado una y otra vez | Parpadeo rápido |

Arranca en *Copia*. Con el botón de grabar se entra y se sale de *Grabar*, y con el de reproducir se entra y se sale de *Reproducir*. Los botones van entre el pin y GND, sin resistencia (se usa el pull-up interno).

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
