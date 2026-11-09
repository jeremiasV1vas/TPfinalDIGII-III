# Programas de la PC

Se conectan a la placa por la UART2 con un adaptador USB a serie de 3,3 V (TX del adaptador a P0.11, RX a P0.10 y GND con GND). Los mensajes están explicados en [`firmware/README.md`](../firmware/README.md#mensajes-con-la-pc).

Hace falta Python 3 y estas librerías:

```bash
pip install -r requirements.txt
```

## Interfaz

```bash
python interfaz.py
```

Se elige el puerto (COM3, /dev/ttyUSB0, etc.) y se conecta. Muestra el modo, los potes, los pulsos de los servos y los gráficos de fuerza y corriente de la garra. Desde ahí se cambia de modo, se prende el PID de fuerza, se cambian las ganancias y el umbral de la protección, y en modo manual se mueven los servos con los deslizadores. *Guardar CSV* guarda todo lo que llegó desde que se conectó, para hacer los gráficos del informe.

### Calibrar el maestro

1. Pasar a modo manual y mover cada servo del esclavo con su deslizador hasta una posición conocida (A). Anotar el pulso.
2. Poner la misma articulación del maestro en esa posición y anotar el valor del pote.
3. Lo mismo para otra posición (B).
4. Copiar los cuatro números en `config.h` (`CAL_BASE_POTE_A`, `CAL_BASE_PULSO_A`, etc.) y volver a grabar la placa.

Para la garra se anotan el pote con la palanca suelta y apretada a fondo, y los pulsos de garra abierta y cerrada (`GARRA_ABIERTA_US` y `GARRA_CERRADA_US`).

## Caracterización corriente-fuerza

```bash
python caracterizar.py COM3
```

Deja la garra quieta y pide que le cuelguen masas conocidas de la punta (empezando por 0 g). Para cada una mide la corriente promedio. Al final ajusta una recta, guarda `caracterizacion.csv`, muestra el gráfico y escribe los dos valores que van en `config.h`: `CORRIENTE_REPOSO_MA` y `GRAMOS_POR_MA`.
