# ARM (Brazo) Cortex M3

> **Asignatura:** Electrónica Digital III - Universidad Nacional de Córdoba
>
> **Profesor:** Blasco
>
> **Integrantes:**
> * Biancucci, Thiago
> * Rivera, Alex Joel
> * Vivas, Jeremías

---

## 🚀 1. Descripción General del Proyecto

Son dos brazos robóticos de cuatro articulaciones (base, hombro, codo y garra) controlados por una LPC1769. El **brazo maestro** no tiene motores: en cada articulación tiene un potenciómetro, y el usuario lo mueve con la mano. El **brazo esclavo** tiene un servo en cada articulación y copia en tiempo real lo que hace el maestro. Además, el esclavo puede grabar un movimiento y repetirlo solo, y la placa mide cuánto aprieta la garra a partir de la corriente de su servo.

Resuelve el problema de manejar un brazo a distancia de forma intuitiva: en vez de mover cada motor con botones o comandos, se mueve un brazo igual al que se quiere controlar. Está pensado como un prototipo didáctico de teleoperación (el mismo principio que se usa para manipular objetos peligrosos o en cirugía asistida), y para enseñar ADC, DMA, PWM, interrupciones y comunicación serie con un ejemplo que se ve.

### 🎯 Alcances del Proyecto

* **El sistema SÍ es capaz de:**
  * Leer los 4 potes del maestro con el ADC en modo *burst* (promediando lo que se juntó en cada período de 20 ms) y mover los 4 servos del esclavo con PWM a 50 Hz.
  * Funcionar en tres modos, que se eligen con botones por interrupción externa:
    * **Copia:** el esclavo sigue al maestro.
    * **Grabar:** copia y además guarda hasta 30 segundos de movimiento.
    * **Reproducir:** repite lo grabado en bucle, sin hacerle caso al maestro.
  * Pasar de un modo a otro con una transición de 1 segundo, para que el esclavo no pegue saltos.
  * Calibrar cada articulación con dos puntos (lectura del pote y pulso del servo) y limitar cada servo a un pulso mínimo y máximo, así no pasa de sus topes aunque el pote siga girando.
  * Medir la carga de la garra: el ADC lee por DMA, todo el tiempo, la tensión del servo de la garra después de una resistencia de sensado. Con un tercer botón se toma una referencia con la garra libre, y la carga se calcula contra esa referencia.
  * Mandar el estado completo a la PC por UART 25 veces por segundo y mostrarlo en una página web, con un gráfico por servo y una animación 3D del brazo.
  * Mostrar el modo con el LED de la placa.
* **En desarrollo para la entrega:**
  * Lazo de control (PI) de la fuerza de agarre de la garra, usando la carga medida.
  * Protección por sobrecorriente con un comparador y el umbral puesto por el DAC.
  * Comandos desde la PC hacia la placa.
* **El sistema NO incluye (Fuera de alcance):**
  * Realimentación de posición de los servos: el esclavo va adonde se le manda, pero no se mide si llegó.
  * Medición de fuerza en newtons: la carga se mide en cuentas del ADC (unos 6 mA por cuenta), sin un sensor de fuerza.
  * Guardar las secuencias al apagar: se graban en RAM.
  * Comunicación inalámbrica.

### ⏩ Posibles Etapas Siguientes

* Pasar el circuito de la protoboard a un PCB con la fuente, los conectores de los servos y la medición de corriente en la misma placa.
* Guardar varias secuencias en una memoria no volátil (EEPROM o la flash del micro) para que no se pierdan al apagar.
* Usar servos con realimentación de posición, o encoders, para cerrar el lazo de posición de cada articulación.
* Medir la fuerza de la garra con un sensor de fuerza (FSR o celda de carga) en vez de inferirla por la corriente.
* Manejar el esclavo de forma inalámbrica (Bluetooth o Wi-Fi), con el maestro en otro lugar.

---

## 📐 2. Arquitectura del Sistema: Hardware y Software

### 🔌 Hardware & Interconexión

* **Diagrama de Bloques:**

```mermaid
flowchart LR
    M["Brazo maestro<br>4 potes"] -- "AD0.0, AD0.1, AD0.2, AD0.4" --> LPC["LPC1769<br>LPCXpresso"]
    B["Botones<br>grabar, reproducir, referencia"] -- "EINT1, EINT2, EINT3" --> LPC
    LPC -- "PWM1.1 a PWM1.3" --> E["Esclavo: servos MG90S<br>base, hombro y codo"]
    LPC -- "PWM1.4" --> G["Esclavo: servo MG90S<br>de la garra"]
    F["Fuente 5 V"] --> E
    F --> R["R_SENSE 0,33 Ω<br>y divisor"]
    R --> G
    R -- "AD0.5 + DMA" --> LPC
    LPC -- "UART2 115200" --> PC["PC<br>interfaz.html"]
    LPC --> L["LED RGB"]
```

* **Conexiones:**

| Qué | Pin del micro | Pin de la placa |
|---|---|---|
| Pote base | P0.23 · AD0.0 | J2-15 |
| Pote hombro | P0.24 · AD0.1 | J2-16 |
| Pote codo | P0.25 · AD0.2 | J2-17 |
| Pote garra (palanca) | P1.30 · AD0.4 | J2-19 |
| Puntas de los 4 potes | 3,3 V / GND | J2-28 / J2-1 |
| Divisor de la garra | P1.31 · AD0.5 | J2-20 |
| Servo base | P2.0 · PWM1.1 | J2-42 |
| Servo hombro | P2.1 · PWM1.2 | J2-43 |
| Servo codo | P2.2 · PWM1.3 | J2-44 |
| Servo garra | P2.3 · PWM1.4 | J2-45 |
| Botón grabar | P2.11 · EINT1 | J2-52 |
| Botón reproducir | P2.12 · EINT2 | J2-53 |
| Botón referencia de la garra | P2.13 · EINT3 | J2-27 |
| TX hacia la PC | P0.10 · TXD2 | J2-40 |
| RX desde la PC | P0.11 · RXD2 | J2-41 |
| Masa común con la fuente de los servos | GND | J2-54 |
| LED rojo / verde | P0.22 / P3.25 | En la placa |

* **Descripción del Circuito y Consideraciones de Diseño:**
  * **Potes del maestro:** cada uno es un divisor entre 3,3 V y GND, con el cursor al ADC. La lectura depende solo de la posición y no del valor del pote, por eso se pueden usar potes de distinto valor (tenemos uno de 5K, uno de 10K y dos de 100K), siempre que sean lineales. El ADC de la LPC pide una fuente de 7,5 kΩ como máximo, y un pote de 100K llega a 25 kΩ en la mitad del recorrido, así que a esos se les pone un capacitor de 100 nF entre el cursor y GND, cerca de la placa.
  * **Servos:** se alimentan de una fuente de 5 V aparte y no de la placa, porque cada MG90S puede pedir cientos de mA. La masa de la fuente se une a un GND de la placa para que el PWM tenga la misma referencia. La señal de 3,3 V de la LPC alcanza para los servos.
  * **Medición de la carga de la garra:** el servo de la garra se alimenta a través de R_SENSE (tres resistencias de 1 Ω y 1/4 W en paralelo, 0,33 Ω) puesta entre los +5 V y el servo. Un divisor de 3,3k y 2,2k baja la tensión del servo (SRV) a 0,4 × SRV, unos 2,0 V sin carga, para que entre en el rango del ADC. A la entrada del ADC hay un capacitor de 100 nF que filtra los picos del servo, y un zener de 3,1 V que protege el pin si algo sale mal. Cuando la garra aprieta, el servo pide más corriente, cae más tensión en R_SENSE y la lectura baja: unos 6 mA por cuenta del ADC. Como la lectura también se mueve con la fuente de 5 V (medimos 5,11 V en vacío y unos 4,75 V forzando la garra), no se usa sola sino contra una referencia tomada con la garra libre.
  * **Botones:** van entre el pin y GND, sin resistencia, porque se usa el pull-up interno. No se usa el EINT0 (P2.10) porque, si ese pin está en 0 cuando arranca la placa, entra al modo de grabación (ISP) y no corre el programa.
  * **UART:** se usa un adaptador USB-serie de 3,3 V.

### 💻 Arquitectura de Software (Firmware)

* **Máquina de Estados:**

```mermaid
stateDiagram-v2
    [*] --> Copia
    Copia --> Grabar: botón grabar
    Grabar --> Copia: botón grabar o memoria llena
    Copia --> Reproducir: botón reproducir, si hay algo grabado
    Reproducir --> Copia: botón reproducir
```

* **Lazo principal:** el SysTick interrumpe cada 20 ms (un período de los servos) y solo cuenta ticks. El `while` del `main` junta lecturas de los potes todo el tiempo, y cada vez que cambia el tick hace un ciclo:
  1. Saca el promedio de cada pote con lo que se juntó en esos 20 ms.
  2. Atiende los botones. Las interrupciones externas solo avisan que hubo un flanco, y el antirrebote se hace por ticks.
  3. Según el modo, calcula adónde tiene que ir el esclavo: desde el maestro (copia y grabar) o desde la secuencia grabada (reproducir). En grabar, además guarda el paso.
  4. Mueve los servos, pasando por la transición de 1 s si se acaba de cambiar de modo.
  5. Actualiza el LED, y cada 2 ticks manda el estado a la PC.

  Mientras tanto, el DMA copia cada conversión del canal 5 (la garra) en un buffer circular de 400 lecturas, sin usar la CPU. Cuando se necesita la carga, se promedia ese buffer.

* **Archivos:** el detalle de cada archivo del firmware está en [`firmware/README.md`](firmware/README.md). La página para la PC está en [`pc/interfaz.html`](pc/interfaz.html): se abre en Chrome o Edge, se conecta al adaptador con Web Serial y lee líneas de este formato:

  ```text
  $T,modo,pote_base,pote_hombro,pote_codo,pote_garra,pulso_base,pulso_hombro,pulso_codo,pulso_garra,pasos,divisor_garra,carga_garra
  ```

---

## ⚡ 3. Especificaciones Eléctricas, Alimentación y Entorno

### 🔌 Parámetros de Alimentación y Consumo

* **Tensión de operación del sistema:** 3,3 V para el micro, los potes y el ADC; 5 V para los servos.
* **Método de alimentación:** la placa LPCXpresso se alimenta por el USB de la PC (el mismo que se usa para grabarla). Los servos usan una fuente externa de 5 V, con la masa unida a la de la placa.
* **Consumo estimado:**
  * Servos en reposo: unos 10 mA cada uno.
  * Servos moviéndose: de 100 a 250 mA cada uno. Con los cuatro a la vez hay picos de alrededor de 1 A, así que la fuente tiene que dar por lo menos 2 A.
  * Garra trabada apretando: hasta unos 700 mA.
  * Modo bajo consumo: no aplica.

### 📌 Electrónica Digital III (Cortex-M / ARM)

* **IDE y SDK:** MCUXpresso IDE, con CMSIS Core v2.00 para LPC17xx (`CMSISv2p00_LPC17xx`) y los drivers de la cátedra (LPC17xx CMSIS Driver Enhancement, 2026).
* **Microcontrolador Principal:** NXP LPC1769 (Cortex-M3 a 100 MHz) en la placa LPCXpresso.
* **Bibliotecas de Terceros y Versiones:** ninguna además de CMSIS y los drivers de la cátedra. La interfaz de la PC es una sola página HTML sin bibliotecas externas (usa la API Web Serial del navegador).
* **Periféricos Avanzados Utilizados:**
  * **ADC:** en modo *burst*, 5 canales, unas 100 mil conversiones por segundo entre todos.
  * **GPDMA:** canal 0, del ADC a la memoria, con una lista enlazada que apunta a sí misma para que el buffer de la garra se llene sin parar.
  * **PWM1:** 4 salidas de un solo flanco a 50 Hz, con pulsos de 1 µs de resolución.
  * **SysTick:** el tick de 20 ms del lazo principal.
  * **NVIC:** EINT1, EINT2 y EINT3 por flanco de bajada para los botones.
  * **UART2:** 115200 8N1 con FIFO.
  * **GPIO:** para el LED.
  * **DAC:** se va a usar para el umbral de la protección por sobrecorriente.
* **Estrategia de Concurrencia:** *bare-metal*, sin RTOS. Hay un lazo principal cooperativo con una máquina de estados (los modos) que se ejecuta una vez por tick del SysTick. Las interrupciones son cortas: el SysTick solo cuenta, y los EINT solo levantan un aviso que se atiende en el siguiente ciclo. La adquisición de la corriente de la garra la hace el DMA en segundo plano.

---

## 🔄 4. Proceso de Integración y Desarrollo

* **Etapa 1 (Validación inicial):** estructura del repo, drivers de la cátedra y LED de la placa parpadeando para comprobar que el proyecto compila y graba.
* **Etapa 2 (Actuadores y adquisición):** servos con PWM1 y lectura de los potes con el ADC. Después agregamos el promedio por tick y una zona muerta en los pulsos, porque con una sola lectura del ADC los servos temblaban. Los servos también los probamos sueltos con un Arduino Uno (barrido de 60° a 120° y ángulos pedidos por el monitor serie), para descartar problemas de los servos antes de montarlos.
* **Etapa 3 (Integración lógica):** modos de copia, grabar y reproducir con los botones por EINT1 y EINT2, transición de 1 s entre modos y LED de color según el modo.
* **Etapa 4 (Comunicación):** UART2 transmitiendo el estado. Primero se probó con un puente entre TX y RX (*loopback*) y una terminal serie, y después con la página `interfaz.html`, a la que se le sumó la animación 3D del brazo.
* **Etapa 5 (Mecánica):** impresión 3D de los dos brazos. Para el maestro rediseñamos las piezas con las medidas reales de los potes, con una pieza de prueba para elegir el agujero del eje. Después armamos los dos brazos y medimos el rango de cada articulación.
* **Etapa 6 (Medición de la garra):** circuito de R_SENSE y divisor aprobado por el profesor, lectura por DMA del AD0.5 y botón de referencia por EINT3.
* **Próximas:** calibración fina de cada articulación con los brazos armados, lazo de control de la fuerza de la garra y protección por sobrecorriente.

---

## 📊 5. Ensayos, Pruebas y Resultados

* **Pruebas Funcionales Realizadas:**
  * **Servos:** cada servo se movió solo, con un Arduino, de 60° a 120° y a ángulos pedidos por el monitor serie, antes de montarlo en el esclavo.
  * **Potes y rango del maestro:** con los brazos armados, cada articulación del maestro recorre de 0° a 180°, con el pote cerca de la mitad (unas 2048 cuentas) con el brazo erguido. Como el pote gira 270° en 4096 cuentas, son unas 15 cuentas por grado: 0° cae cerca de 683 y 180° cerca de 3413.
  * **Ajuste del eje del pote:** con la pieza de prueba, el agujero de 6,0 mm entra justo y el de 6,2 mm queda casi suelto. Se eligió 6,0 mm.
  * **UART:** con el puente entre TX y RX y una terminal serie se revisó el cableado, y después la interfaz recibió las líneas de estado de la placa a 115200.
  * **Fuente de los servos:** 5,11 V en vacío y unos 4,75 V forzando la garra. Esa caída es la razón por la que la carga de la garra se mide contra una referencia y no con la lectura sola.
* **Evidencia Fotográfica y Gráficos:**
  * *Capturas de instrumental:* `![Interfaz con datos de la placa](docs/captura_interfaz.png)`
  * *Foto del Prototipo Real:* `![Brazos maestro y esclavo](docs/foto_brazos.jpg)`

---

## 📂 6. Estructura del Repositorio

```text
├── firmware/                 # Proyecto de MCUXpresso
│   ├── CMSISv2p00_LPC17xx/   # CMSIS y drivers de la cátedra
│   ├── src/                  # Nuestro código (.c)
│   ├── inc/                  # Nuestros headers (.h); los pines están en config.h
│   └── README.md             # Qué hace cada archivo y cómo importar el proyecto
├── pc/
│   └── interfaz.html         # Página para ver el estado de los brazos desde la PC
├── hardware/                 # Esquemático y lista de componentes
├── docs/                     # Imágenes del README
└── README.md                 # Este archivo
```
