"""@file interfaz.py
@brief Interfaz para ver y manejar los brazos desde la PC.

Muestra el estado de la placa y grafica la fuerza y la corriente de la garra.
Desde acá se cambia de modo, se ajusta el PID y el umbral de la protección,
y en modo manual se mueven los servos a mano.

Uso: python interfaz.py
"""

import csv
import queue
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure

from placa import (MODO_COPIA, MODO_GRABAR, MODO_MANUAL, MODO_REPRODUCIR, MODOS, CAMPOS,
                   Placa, puertos_disponibles)

## Segundos que se ven en los gráficos.
VENTANA_S = 10
## Cada cuánto se actualiza la pantalla, en ms.
REFRESCO_MS = 100
## Estados por segundo que manda la placa.
ESTADOS_POR_S = 25

## Nombre que se muestra y nombre en los campos de la placa.
ARTICULACIONES = [("Base", "base"), ("Hombro", "hombro"), ("Codo", "codo"), ("Garra", "garra")]

## Ganancias con las que arranca el firmware (config.h).
PID_INICIAL = (0.05, 2.0, 0.0)
UMBRAL_INICIAL_MA = 600


class Interfaz:
    """@brief Ventana principal."""

    def __init__(self, raiz):
        self.raiz = raiz
        self.placa = None
        ## Todos los estados desde que se conectó, para guardar el CSV.
        self.historial = []
        ## Último pulso que se mandó con cada deslizador en modo manual.
        self.pulso_enviado = [None] * 4

        raiz.title("Brazos maestro-esclavo")
        raiz.protocol("WM_DELETE_WINDOW", self.salir)

        izquierda = ttk.Frame(raiz, padding=6)
        izquierda.grid(row=1, column=0, sticky="n")
        derecha = ttk.Frame(raiz, padding=6)
        derecha.grid(row=1, column=1, sticky="nsew")
        raiz.columnconfigure(1, weight=1)
        raiz.rowconfigure(1, weight=1)

        self.armar_conexion()
        self.armar_estado(izquierda)
        self.armar_modos(izquierda)
        self.armar_pid(izquierda)
        self.armar_manual(izquierda)
        self.armar_graficos(derecha)
        self.armar_mensajes()

        self.raiz.after(REFRESCO_MS, self.refrescar)

    # ------------------------------------------------------------------
    # Armado de la ventana
    # ------------------------------------------------------------------

    def armar_conexion(self):
        marco = ttk.Frame(self.raiz, padding=6)
        marco.grid(row=0, column=0, columnspan=2, sticky="ew")

        ttk.Label(marco, text="Puerto:").pack(side="left")
        self.puerto = ttk.Combobox(marco, width=18, values=puertos_disponibles())
        self.puerto.pack(side="left", padx=4)
        if self.puerto["values"]:
            self.puerto.current(0)
        ttk.Button(marco, text="Buscar", command=self.buscar_puertos).pack(side="left")
        self.boton_conectar = ttk.Button(marco, text="Conectar", command=self.conectar)
        self.boton_conectar.pack(side="left", padx=4)
        ttk.Button(marco, text="Guardar CSV", command=self.guardar_csv).pack(side="right")

    def armar_estado(self, padre):
        marco = ttk.LabelFrame(padre, text="Estado", padding=6)
        marco.pack(fill="x")

        ttk.Label(marco, text="Modo:").grid(row=0, column=0, sticky="w")
        self.texto_modo = ttk.Label(marco, text="-", font=("TkDefaultFont", 11, "bold"))
        self.texto_modo.grid(row=0, column=1, columnspan=2, sticky="w")

        ttk.Label(marco, text="Garra:").grid(row=1, column=0, sticky="w")
        self.texto_garra = ttk.Label(marco, text="-")
        self.texto_garra.grid(row=1, column=1, columnspan=2, sticky="w")

        ttk.Label(marco, text="Pote").grid(row=2, column=1)
        ttk.Label(marco, text="Pulso (µs)").grid(row=2, column=2)
        self.texto_pote = {}
        self.texto_pulso = {}
        for fila, (nombre, clave) in enumerate(ARTICULACIONES, start=3):
            ttk.Label(marco, text=nombre).grid(row=fila, column=0, sticky="w")
            self.texto_pote[clave] = ttk.Label(marco, text="-", width=7, anchor="e")
            self.texto_pote[clave].grid(row=fila, column=1)
            self.texto_pulso[clave] = ttk.Label(marco, text="-", width=7, anchor="e")
            self.texto_pulso[clave].grid(row=fila, column=2)

        self.texto_valores = ttk.Label(marco, text="", justify="left")
        self.texto_valores.grid(row=7, column=0, columnspan=3, sticky="w", pady=(6, 0))

    def armar_modos(self, padre):
        marco = ttk.LabelFrame(padre, text="Modo", padding=6)
        marco.pack(fill="x", pady=6)

        botones = [("Copia", MODO_COPIA), ("Grabar", MODO_GRABAR),
                   ("Reproducir", MODO_REPRODUCIR), ("Manual", MODO_MANUAL)]
        for columna, (texto, modo) in enumerate(botones):
            ttk.Button(marco, text=texto, width=10,
                       command=lambda m=modo: self.cambiar_modo(m)).grid(row=0, column=columna)

        self.fuerza_on = tk.IntVar(value=0)
        ttk.Checkbutton(marco, text="Garra con PID de fuerza", variable=self.fuerza_on,
                        command=self.cambiar_fuerza).grid(row=1, column=0, columnspan=4,
                                                          sticky="w", pady=(6, 0))

    def armar_pid(self, padre):
        marco = ttk.LabelFrame(padre, text="PID y protección", padding=6)
        marco.pack(fill="x")

        self.ganancias = []
        for columna, (nombre, valor) in enumerate(zip(("Kp", "Ki", "Kd"), PID_INICIAL)):
            ttk.Label(marco, text=nombre).grid(row=0, column=2 * columna, padx=(4, 2))
            variable = tk.StringVar(value=str(valor))
            ttk.Entry(marco, textvariable=variable, width=7).grid(row=0, column=2 * columna + 1)
            self.ganancias.append(variable)
        ttk.Button(marco, text="Enviar", command=self.enviar_pid).grid(row=0, column=6, padx=4)

        ttk.Label(marco, text="Umbral (mA)").grid(row=1, column=0, columnspan=2, pady=(6, 0))
        self.umbral = tk.StringVar(value=str(UMBRAL_INICIAL_MA))
        ttk.Entry(marco, textvariable=self.umbral, width=7).grid(row=1, column=2, columnspan=2,
                                                                 pady=(6, 0), sticky="w")
        ttk.Button(marco, text="Enviar", command=self.enviar_umbral).grid(row=1, column=6,
                                                                          pady=(6, 0))

    def armar_manual(self, padre):
        marco = ttk.LabelFrame(padre, text="Modo manual (pulso en µs)", padding=6)
        marco.pack(fill="x", pady=6)

        self.deslizadores = []
        for fila, (nombre, _) in enumerate(ARTICULACIONES):
            ttk.Label(marco, text=nombre).grid(row=fila, column=0, sticky="w")
            variable = tk.IntVar(value=1500)
            tk.Scale(marco, from_=500, to=2500, orient="horizontal", length=260,
                     resolution=5, variable=variable).grid(row=fila, column=1)
            self.deslizadores.append(variable)

    def armar_graficos(self, padre):
        self.figura = Figure(figsize=(7, 5.5), dpi=90)
        self.eje_fuerza = self.figura.add_subplot(2, 1, 1)
        self.eje_corriente = self.figura.add_subplot(2, 1, 2, sharex=self.eje_fuerza)

        self.linea_referencia, = self.eje_fuerza.plot([], [], label="Pedida")
        self.linea_fuerza, = self.eje_fuerza.plot([], [], label="Medida")
        self.eje_fuerza.set_ylabel("Fuerza (g)")
        self.eje_fuerza.legend(loc="upper left")

        self.linea_corriente, = self.eje_corriente.plot([], [], label="Corriente")
        self.linea_umbral, = self.eje_corriente.plot([], [], "--", label="Umbral")
        self.eje_corriente.set_ylabel("Corriente (mA)")
        self.eje_corriente.set_xlabel("Tiempo (s)")
        self.eje_corriente.legend(loc="upper left")

        for eje in (self.eje_fuerza, self.eje_corriente):
            eje.set_xlim(-VENTANA_S, 0)
            eje.grid(True, alpha=0.3)
        self.figura.tight_layout()

        self.lienzo = FigureCanvasTkAgg(self.figura, master=padre)
        self.lienzo.get_tk_widget().pack(fill="both", expand=True)

    def armar_mensajes(self):
        self.mensajes = tk.Text(self.raiz, height=6, state="disabled")
        self.mensajes.grid(row=2, column=0, columnspan=2, sticky="ew", padx=6, pady=(0, 6))
        self.mensajes.tag_configure("falla", foreground="red")

    # ------------------------------------------------------------------
    # Acciones
    # ------------------------------------------------------------------

    def anotar(self, texto, etiqueta=None):
        self.mensajes.configure(state="normal")
        self.mensajes.insert("end", texto + "\n", etiqueta)
        self.mensajes.see("end")
        self.mensajes.configure(state="disabled")

    def buscar_puertos(self):
        self.puerto["values"] = puertos_disponibles()
        if self.puerto["values"]:
            self.puerto.current(0)

    def conectar(self):
        if self.placa is not None:
            self.placa.cerrar()
            self.placa = None
            self.boton_conectar.configure(text="Conectar")
            self.anotar("Desconectado")
            return

        try:
            self.placa = Placa(self.puerto.get())
        except Exception as error:
            messagebox.showerror("No se pudo conectar", str(error))
            return
        self.historial = []
        self.boton_conectar.configure(text="Desconectar")
        self.anotar("Conectado a " + self.puerto.get())

    def enviar(self, comando):
        if self.placa is None:
            self.anotar("Primero hay que conectarse")
            return
        self.placa.enviar(comando)

    def cambiar_modo(self, modo):
        # Al pasar a manual, los deslizadores arrancan donde están los servos,
        # así no pegan un salto
        if modo == MODO_MANUAL and self.placa is not None and self.placa.estado is not None:
            for i, (_, clave) in enumerate(ARTICULACIONES):
                pulso = self.placa.estado["pulso_" + clave]
                self.deslizadores[i].set(pulso)
                self.pulso_enviado[i] = pulso
        self.enviar("M,%d" % modo)

    def cambiar_fuerza(self):
        self.enviar("F,%d" % self.fuerza_on.get())

    def enviar_pid(self):
        try:
            kp, ki, kd = [float(v.get().replace(",", ".")) for v in self.ganancias]
        except ValueError:
            messagebox.showerror("PID", "Las ganancias tienen que ser números")
            return
        # La placa las recibe multiplicadas por 1000
        self.enviar("K,%d,%d,%d" % (round(kp * 1000), round(ki * 1000), round(kd * 1000)))

    def enviar_umbral(self):
        try:
            umbral = int(self.umbral.get())
        except ValueError:
            messagebox.showerror("Umbral", "El umbral tiene que ser un número entero de mA")
            return
        self.enviar("U,%d" % umbral)

    def mandar_deslizadores(self):
        """@brief En modo manual, manda los deslizadores que se movieron."""
        estado = self.placa.estado
        if estado is None or estado["modo"] != MODO_MANUAL:
            return
        for i, variable in enumerate(self.deslizadores):
            pulso = variable.get()
            if pulso != self.pulso_enviado[i]:
                self.placa.enviar("S,%d,%d" % (i + 1, pulso))
                self.pulso_enviado[i] = pulso

    def guardar_csv(self):
        if not self.historial:
            messagebox.showinfo("Guardar CSV", "Todavía no llegó nada de la placa")
            return
        nombre = filedialog.asksaveasfilename(defaultextension=".csv",
                                              filetypes=[("CSV", "*.csv")])
        if not nombre:
            return
        inicio = self.historial[0]["t"]
        with open(nombre, "w", newline="") as archivo:
            escritor = csv.writer(archivo)
            escritor.writerow(["tiempo_s"] + CAMPOS)
            for estado in self.historial:
                escritor.writerow(["%.3f" % (estado["t"] - inicio)] + [estado[c] for c in CAMPOS])
        self.anotar("Guardado " + nombre)

    # ------------------------------------------------------------------
    # Actualización periódica
    # ------------------------------------------------------------------

    def refrescar(self):
        if self.placa is not None:
            while True:
                try:
                    self.historial.append(self.placa.telemetria.get_nowait())
                except queue.Empty:
                    break

            while True:
                try:
                    mensaje = self.placa.mensajes.get_nowait()
                except queue.Empty:
                    break
                if mensaje.startswith("$FALLA"):
                    self.anotar("¡Saltó la protección de sobrecorriente! La garra se abrió.", "falla")
                else:
                    self.anotar(mensaje)

            if self.placa.estado is not None:
                self.mostrar_estado(self.placa.estado)
                self.mandar_deslizadores()
            self.actualizar_graficos()

        self.raiz.after(REFRESCO_MS, self.refrescar)

    def mostrar_estado(self, estado):
        self.texto_modo.configure(text=MODOS[estado["modo"]],
                                  foreground="red" if estado["modo"] == 4 else "")
        self.texto_garra.configure(text="PID de fuerza" if estado["fuerza_on"] else "Posición")
        for _, clave in ARTICULACIONES:
            self.texto_pote[clave].configure(text=str(estado["pote_" + clave]))
            self.texto_pulso[clave].configure(text=str(estado["pulso_" + clave]))
        self.texto_valores.configure(text=(
            "Corriente: %d mA   Umbral: %d mA\n"
            "Fuerza: %d g   Pedida: %d g\n"
            "Pasos grabados: %d (%.1f s)" % (
                estado["corriente"], estado["umbral"], estado["fuerza"], estado["referencia"],
                estado["pasos"], estado["pasos"] * 0.02)))

    def actualizar_graficos(self):
        if not self.historial:
            return
        recientes = self.historial[-VENTANA_S * ESTADOS_POR_S:]
        ahora = recientes[-1]["t"]
        tiempos = [e["t"] - ahora for e in recientes]

        self.linea_referencia.set_data(tiempos, [e["referencia"] for e in recientes])
        self.linea_fuerza.set_data(tiempos, [e["fuerza"] for e in recientes])
        self.linea_corriente.set_data(tiempos, [e["corriente"] for e in recientes])
        self.linea_umbral.set_data(tiempos, [e["umbral"] for e in recientes])

        for eje in (self.eje_fuerza, self.eje_corriente):
            eje.relim()
            eje.autoscale_view(scalex=False)
        self.lienzo.draw_idle()

    def salir(self):
        if self.placa is not None:
            self.placa.cerrar()
        self.raiz.destroy()


if __name__ == "__main__":
    raiz = tk.Tk()
    Interfaz(raiz)
    raiz.mainloop()
