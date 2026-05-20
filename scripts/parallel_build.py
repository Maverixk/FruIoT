"""
PlatformIO post-script: forza la compilazione parallela usando tutti i core
disponibili della CPU. Equivalente a `pio run -j <num_core>` ma applicato
automaticamente ad ogni build, anche dalla GUI di VSCode.

Riferimento: https://docs.platformio.org/en/latest/scripting/index.html
"""

import multiprocessing

# Import del PlatformIO build environment
Import("env")  # noqa: F821 — fornito a runtime da SCons/PlatformIO

# Numero di job paralleli = numero di core fisici/logici della CPU.
# Si può forzare un numero diverso modificando questa riga.
num_jobs = multiprocessing.cpu_count()

# Applica l'opzione a SCons (il build system sotto PlatformIO)
env.SetOption("num_jobs", num_jobs)  # noqa: F821

print(f"[parallel_build] Compilazione parallela attiva su {num_jobs} job.")
