"""@file caracterizar.py
@brief Caracterización corriente-fuerza de la garra del esclavo.

Se deja la garra quieta en una posición y se le van colgando masas conocidas
de la punta con un hilo. Para cada masa se mide la corriente promedio del
servo. Al final se ajusta una recta y se muestran los dos números que van en
config.h (CORRIENTE_REPOSO_MA y GRAMOS_POR_MA).

Uso: python caracterizar.py COM3
     python caracterizar.py /dev/ttyUSB0 --pulso 1500
"""

import argparse
import csv
import queue
import time

import matplotlib.pyplot as plt
import numpy as np

from placa import MODO_COPIA, MODO_MANUAL, Placa, puertos_disponibles

## Segundos que se espera después de colgar la masa, para que se quede quieta.
ESPERA_S = 2
## Segundos durante los que se promedia la corriente.
MEDICION_S = 3


def medir_corriente(placa, segundos):
    """@brief Promedio de la corriente que manda la placa durante un rato, en mA."""
    # Se tira lo que había en la cola, que es de antes
    while not placa.telemetria.empty():
        placa.telemetria.get()

    valores = []
    fin = time.time() + segundos
    while time.time() < fin:
        try:
            valores.append(placa.telemetria.get(timeout=0.5)["corriente"])
        except queue.Empty:
            pass

    if not valores:
        return None
    return sum(valores) / len(valores)


def avisos(placa):
    """@brief Muestra lo que haya mandado la placa además del estado."""
    while not placa.mensajes.empty():
        mensaje = placa.mensajes.get()
        if mensaje.startswith("$FALLA"):
            print("  ¡Saltó la protección! Subí el umbral (U,mA) o usá menos masa.")
        elif mensaje.startswith("$ERROR"):
            print("  La placa no entendió:", mensaje)


def main():
    parser = argparse.ArgumentParser(description="Caracterización corriente-fuerza de la garra")
    parser.add_argument("puerto", nargs="?", help="Puerto serie de la placa")
    parser.add_argument("--pulso", type=int, default=1500,
                        help="Pulso en µs en el que se deja la garra (default 1500)")
    parser.add_argument("--salida", default="caracterizacion.csv",
                        help="Archivo donde se guardan las mediciones")
    args = parser.parse_args()

    puerto = args.puerto
    if puerto is None:
        print("Puertos disponibles:", ", ".join(puertos_disponibles()) or "ninguno")
        puerto = input("Puerto: ").strip()

    placa = Placa(puerto)
    time.sleep(0.5)

    # Modo manual, garra por posición, y la garra quieta en el pulso elegido
    placa.enviar("M,%d" % MODO_MANUAL)
    placa.enviar("F,0")
    placa.enviar("S,4,%d" % args.pulso)
    time.sleep(1)
    avisos(placa)

    print()
    print("Garra quieta en %d µs." % args.pulso)
    print("Colgá cada masa de la punta de la garra, escribí cuántos gramos son y dale Enter.")
    print("Empezá con 0 (sin nada colgado). Enter sin nada para terminar.")
    print()

    masas = []
    corrientes = []
    while True:
        texto = input("Masa en gramos: ").strip().replace(",", ".")
        if texto == "":
            break
        try:
            masa = float(texto)
        except ValueError:
            print("  Tiene que ser un número")
            continue

        print("  Esperando que se quede quieta...")
        time.sleep(ESPERA_S)
        corriente = medir_corriente(placa, MEDICION_S)
        avisos(placa)
        if corriente is None:
            print("  No llegó nada de la placa, revisá la conexión")
            continue

        print("  %.0f g -> %.1f mA" % (masa, corriente))
        masas.append(masa)
        corrientes.append(corriente)

    placa.enviar("M,%d" % MODO_COPIA)
    placa.cerrar()

    if len(masas) < 2:
        print("Hacen falta al menos dos mediciones para sacar la recta.")
        return

    with open(args.salida, "w", newline="") as archivo:
        escritor = csv.writer(archivo)
        escritor.writerow(["masa_g", "corriente_mA"])
        for masa, corriente in zip(masas, corrientes):
            escritor.writerow([masa, "%.2f" % corriente])
    print("\nMediciones guardadas en", args.salida)

    # Recta corriente = pendiente * masa + ordenada. La ordenada es la corriente
    # sin carga, y para pasar de corriente a gramos se divide por la pendiente.
    pendiente, ordenada = np.polyfit(masas, corrientes, 1)
    prediccion = np.polyval([pendiente, ordenada], masas)
    residuo = np.sum((np.array(corrientes) - prediccion) ** 2)
    total = np.sum((np.array(corrientes) - np.mean(corrientes)) ** 2)
    r2 = 1 - residuo / total if total > 0 else 0

    print("\nRecta: corriente = %.4f mA/g * masa + %.2f mA   (R² = %.3f)" % (pendiente, ordenada, r2))
    if pendiente <= 0:
        print("La corriente no sube con la masa: revisá el montaje y la medición.")
    else:
        print("\nPara config.h:")
        print("#define CORRIENTE_REPOSO_MA %.1ff" % ordenada)
        print("#define GRAMOS_POR_MA       %.3ff" % (1 / pendiente))

    plt.plot(masas, corrientes, "o", label="Mediciones")
    eje_x = np.linspace(min(masas), max(masas), 50)
    plt.plot(eje_x, pendiente * eje_x + ordenada, label="Recta (R² = %.3f)" % r2)
    plt.xlabel("Masa colgada (g)")
    plt.ylabel("Corriente del servo (mA)")
    plt.title("Caracterización corriente-fuerza de la garra")
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.show()


if __name__ == "__main__":
    main()
