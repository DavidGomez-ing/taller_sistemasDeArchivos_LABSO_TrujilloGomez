# Taller: sistemas de archivos

Laboratorio de Sistemas Operativos.

## Contenido

| Archivo | Descripción |
|---|---|
| `infoarchivo.c` | Programa en C: informa tamaño, tipo, permisos, enlaces y fecha de modificación |
| `Makefile` | Compila con `gcc -Wall -Wextra` |
| `correr_taller.sh` | Ejecuta los comandos del taller y guarda la salida en `evidencia/evidencia.txt` |
| `BITACORA.md` | Bitácora con observaciones y respuestas (puntos 2, 5, 6, 7 y 9) |

## Uso (Debian)

```bash
sudo apt install build-essential    # si falta gcc/make
make
./infoarchivo archivo.txt
./correr_taller.sh                  # genera evidencia/evidencia.txt
```
