"""@file placa.py
@brief Conexión con la placa por el puerto serie.

La placa manda una línea $T,... 25 veces por segundo con el estado, y contesta
$OK o $ERROR a cada comando. Acá se lee todo en un hilo aparte, así la
interfaz no se traba esperando al puerto.
"""

import queue
import threading
import time

import serial
import serial.tools.list_ports

## Nombres de los valores de la línea $T, en el mismo orden que los manda la placa.
CAMPOS = [
    "modo", "fuerza_on",
    "pote_base", "pote_hombro", "pote_codo", "pote_garra",
    "pulso_base", "pulso_hombro", "pulso_codo", "pulso_garra",
    "referencia", "fuerza", "corriente", "umbral", "pasos",
]

## Nombre de cada modo, por número.
MODOS = ["Copia", "Grabar", "Reproducir", "Manual", "Falla"]

MODO_COPIA = 0
MODO_GRABAR = 1
MODO_REPRODUCIR = 2
MODO_MANUAL = 3
MODO_FALLA = 4


def puertos_disponibles():
    """@brief Lista de puertos serie que hay en la PC (COM3, /dev/ttyUSB0, etc.)."""
    return [p.device for p in serial.tools.list_ports.comports()]


class Placa:
    """@brief Puerto serie abierto con la placa."""

    def __init__(self, puerto, baudios=115200):
        self.serie = serial.Serial(puerto, baudios, timeout=0.1)
        ## Último estado recibido, como diccionario con las claves de CAMPOS.
        self.estado = None
        ## Cada estado que llega, con la hora en "t". Lo va sacando quien lo use.
        self.telemetria = queue.Queue()
        ## Todo lo que no es $T: respuestas a comandos y avisos de falla.
        self.mensajes = queue.Queue()

        self._corriendo = True
        self._hilo = threading.Thread(target=self._leer, daemon=True)
        self._hilo.start()

    def _leer(self):
        while self._corriendo:
            try:
                crudo = self.serie.readline()
            except serial.SerialException:
                self.mensajes.put("Se cortó la conexión con la placa")
                return

            # Todo lo que manda la placa empieza con $. Si no, es un pedazo de
            # línea (por ejemplo, la primera después de conectarse) y se descarta.
            linea = crudo.decode("ascii", errors="ignore").strip()
            if not linea.startswith("$"):
                continue

            if not linea.startswith("$T,"):
                self.mensajes.put(linea)
                continue

            # Si la línea llegó cortada o con basura, se descarta
            valores = linea[3:].split(",")
            if len(valores) != len(CAMPOS):
                continue
            try:
                estado = dict(zip(CAMPOS, [int(v) for v in valores]))
            except ValueError:
                continue

            estado["t"] = time.time()
            self.estado = estado
            self.telemetria.put(estado)

    def enviar(self, comando):
        """@brief Manda un comando, por ejemplo "M,3" o "K,500,5000,0"."""
        self.serie.write((comando + "\n").encode("ascii"))

    def cerrar(self):
        """@brief Para el hilo de lectura y cierra el puerto."""
        self._corriendo = False
        self._hilo.join(timeout=1)
        self.serie.close()
