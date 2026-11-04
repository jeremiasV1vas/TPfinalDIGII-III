# Firmware

```text
firmware/
├── CMSISv2p00_LPC17xx/   # Drivers de la cátedra (LPC17xx-CMSIS-Driver-Enhancement), sin los tests
├── src/                  # Nuestro código (.c)
└── inc/                  # Nuestros headers (.h). Los pines están todos en config.h
```

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

## Compilación en GitHub

Cada PR se compila solo con `arm-none-eabi-gcc` (ver `ci/` y `.github/workflows/build.yml`). Sirve para enterarse rápido si algo no compila, pero el archivo que se graba en la placa sale de MCUXpresso.

Para probarlo en la compu (Linux o WSL): `make -C ci`.
